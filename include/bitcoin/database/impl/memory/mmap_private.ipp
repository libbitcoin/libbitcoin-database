/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_PRIVATE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_PRIVATE_IPP

#include <chrono>
#include <fcntl.h>
#include <mutex>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mman.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/utilities.hpp>

namespace libbitcoin {
namespace database {

// column dispatch, not thread safe.
// ----------------------------------------------------------------------------
// private

TEMPLATE
template <size_t... Index>
bool CLASS::flush_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT
{
    return (flush_<Index>(rows) && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::map_all_(std::index_sequence<Index...>) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    using namespace system;
    const auto page = page_size();

    // Page size must be a power of two.
    if (is_zero(page) || !is_one(ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        capacity_.store(zero);
        return false;
    }

    page_ = page;
    window_.store(zero);
    settled_.store(staged_ ? logical_.load() : zero);
    frontier_.store(staged_ ? logical_.load() : zero);
    marks_.store(zero);
#endif

    if (!(map_<Index>() && ...))
    {
        capacity_.store(zero);
        file_.store(zero);
        return false;
    }

    // The file is provisioned in full; capacity publishes committed rows only.
    file_.store(to_provision());
    capacity_.store(to_commitment());
    check_invariants_();
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::unmap_all_(std::index_sequence<Index...>) NOEXCEPT
{
    const auto capacity = capacity_.load();
    const auto success = (unmap_<Index>(capacity) && ...);
    capacity_.store(zero);

    // Unmapping truncates the file to logical, which is then its provisioning.
    file_.store(logical_.load());

#if defined(MANAGE_STAGING)
    window_.store(zero);
    settled_.store(zero);
    frontier_.store(zero);
    marks_.store(zero);
    dirty_.reset();
    intent_.reset();
    released_.reset();
    sweep_.reset();
    words_ = zero;
    engaged_.store(false);
    lazy_.store(false);
    shared_.store(false);
#endif

    return success;
}

TEMPLATE
template <size_t... Index>
bool CLASS::remap_all_(size_t capacity, std::index_sequence<Index...>,
    bool final) NOEXCEPT
{
    // Probed before touching any column file: a refused wave then retains no
    // surplus provisioning (a partial wave cannot be trimmed, as msc cannot
    // shrink a mapped file).
    if (!probe_(capacity))
    {
        if (final)
        {
            using namespace system;
            set_disk_space(ceilinged_add(headroom_, ceilinged_multiply(
                floored_subtract(capacity, file_.load()), stride)));
        }

        return false;
    }

    if (!(remap_<Index>(capacity, final) && ...))
    {
        // A non-final refusal leaves the maps and capacity intact for the
        // caller's reduced retry (columns already grown by the refused
        // attempt harmlessly retain surplus commitment or provisioning).
        if (final)
            capacity_.store(zero);

        return false;
    }

    // Growth beyond the provisioned file extends it (resize_ is a no-op
    // within), so the extent tracks the high water of provisioning.
    file_.store(std::max(file_.load(), capacity));
    capacity_.store(capacity);
    check_invariants_();
    return true;
}

// Iterated growth (callers hold the remap lock). Growth asks are amortized
// (rate surplus over the necessity), and each is admitted only while it
// leaves the configured headroom of the backing resource unclaimed (probed
// with the ask, released on grant), so exhaustion never consumes the
// system's final bytes. A large amortization step can be refused while the
// necessity fits, so iterate: halve the refused surplus toward the
// necessity. Refusal of the necessity is exhaustion, not store damage:
// published as disk full (space set, store intact, writes fail fast until
// cleared), it clears by settle drainage or operator relief, where teardown
// would convert a shortage into a restore.
TEMPLATE
bool CLASS::grow_(size_t end) NOEXCEPT
{
    if (!is_zero(space_.load()))
        return false;

    using namespace system;
    for (auto extended = to_growth(end);
        !remap_all_(extended, sequence{}, false);
        extended = ceilinged_add(end,
            to_half(floored_subtract(extended, end))))
    {
        if (fault_.load())
            return false;

        if (extended <= end)
        {
            set_disk_space(ceilinged_add(headroom_, ceilinged_multiply(
                floored_subtract(end, capacity_.load()), stride)));
            return false;
        }
    }

    return true;
}

// The wave probe reserves the whole extension plus headroom on the store
// volume (column widths sum to the stride) leaves the headroom unclaimed.
// An unmeasurable volume admits the wave: growth failure is the store's own
// disk full detection, and a query failure is not an exhaustion signal.
TEMPLATE
bool CLASS::probe_(size_t capacity) NOEXCEPT
{
    using namespace system;
    const auto bytes = ceilinged_multiply(
        floored_subtract(capacity, file_.load()), stride);

    if (is_zero(bytes) || is_zero(headroom_))
        return true;

    size_t available{};
    if (!file::space(available, filenames_.front()))
        return true;

    return available >= ceilinged_add(bytes, headroom_);
}

// disk_full: space is set but no code is set with false return.
TEMPLATE
template <size_t Column>
bool CLASS::resize_(size_t size, bool final) NOEXCEPT
{
    // The file is provisioned ahead of commitment, so growth within the
    // provisioned extent requires no disk operation (the space is reserved).
    const auto extent = file_.load();
    if (size <= extent)
        return true;

    using namespace system;
    const auto target = to_width<Column>(size);
    const auto capacity = to_width<Column>(extent);

    // Disk full detection, any other failure is an abort. The wave probe
    // (remap_all_) precedes, so refusal here is a raced foreign consumer.
#if !defined(WITHOUT_FALLOCATE)
    if (::fallocate(opened_[Column], 0, capacity, target - capacity) == fail)
#else
    if (::ftruncate(opened_[Column], target) == fail)
#endif
    {
        // Disk full is the only restartable store failure (leave mapped).
        // A non-final refusal is not published: the caller retries reduced.
        // The published requirement includes the headroom (a retry probes).
        if (errno == ENOSPC)
        {
            if (final)
                set_disk_space(ceilinged_add(headroom_, ceilinged_multiply(
                    floored_subtract(size, extent), stride)));

            return false;
        }

        set_first_code(error::ftruncate_failure);
        unmap_<Column>(capacity_.load());
        return false;
    }

    return true;
}

// worker, instance-owned (load/unload lifecycle).
// ----------------------------------------------------------------------------
// private

TEMPLATE
void CLASS::worker_start_() NOEXCEPT
{
#if defined(MANAGE_STAGING)
    // A shared head has no lazy writer (the kernel writes its mapping back).
    if (!staged_ && head_shared)
        return;

    limit_ = system_memory() / throttle_factor;
#endif

    working_.store(true);
    worker_ = std::thread([this]() NOEXCEPT
    {
        worker_run_();
    });
}

TEMPLATE
void CLASS::worker_stop_() NOEXCEPT
{
    if (!working_.exchange(false))
        return;

#if defined(MANAGE_STAGING)
    signal_();
#endif

    {
        std::unique_lock worker_lock(worker_mutex_);
        worker_cv_.notify_all();
    }

    if (worker_.joinable())
        worker_.join();
}

TEMPLATE
void CLASS::worker_run_() NOEXCEPT
{
#if defined(MANAGE_STAGING)
    staged_ ? body_run_() : head_run_();
#else
    scan_run_();
#endif
}

// One second tick, false when stopped.
TEMPLATE
bool CLASS::tick_() NOEXCEPT
{
    std::unique_lock worker_lock(worker_mutex_);
    worker_cv_.wait_for(worker_lock, std::chrono::seconds(1));
    worker_lock.unlock();
    return working_.load();
}

} // namespace database
} // namespace libbitcoin

#endif
