/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_BODY_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_BODY_IPP

#include <mutex>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/utilities.hpp>

#if defined(MANAGE_STAGING)

namespace libbitcoin {
namespace database {

// staged body dispatch, not thread safe.
// ----------------------------------------------------------------------------
// private

TEMPLATE
template <size_t... Index>
bool CLASS::settle_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT
{
    const auto from = settled_.load();
    if (!(settle_<Index>(from, rows) && ...))
        return false;

    settled_.store(rows);
    signal_();
    check_invariants_();
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::unsettle_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT
{
    if (!(unsettle_<Index>(rows) && ...))
        return false;

    settled_.store(rows);
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::evict_all_(size_t from, size_t to,
    std::index_sequence<Index...>) NOEXCEPT
{
    return (evict_<Index>(from, to) && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::settle_write_(size_t from, size_t to,
    std::index_sequence<Index...>) NOEXCEPT
{
    return (pwrite_all(opened_[Index],
        std::next(memory_map_[Index], to_width<Index>(from)),
        to_width<Index>(to) - to_width<Index>(from),
        to_width<Index>(from)) && ...);
}

// staged body wrappers, not thread safe.
// ----------------------------------------------------------------------------
// private

// Convert flushed rows [from, to) to a read-only shared file mapping, page
// floored so the settle boundary page remains anonymous with its settled
// bytes retained. Releases the covered anonymous pages. Failure results in
// unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::settle_(size_t from, size_t to) NOEXCEPT
{
    if (!staged_)
        return true;

    const auto begin = page_floor(to_width<Column>(from));
    const auto end = page_floor(to_width<Column>(to));
    if (begin == end)
        return true;

    const auto address = std::next(memory_map_[Column], begin);
    if (mmap_settle(address, end - begin, opened_[Column], begin) == fail)
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

#if !defined(WITHOUT_MADVISE)
    if (!advise_(address, end - begin))
    {
        teardown_<Column>(error::madvise_failure);
        return false;
    }

    // Demote the settled extent to reclaim-first order: cached for imminent
    // re-read (validation follows archival) but reclaimed under pressure
    // before active pages and anonymous heads (head residency priority).
    // Ample hosts skip demotion: with nothing contesting memory it only
    // converts warm cache into re-read faults (see demote_memory).
    static const auto ample = system_memory() > demote_memory;
    if (!ample && (mmap_cold(address, end - begin) == fail))
    {
        teardown_<Column>(error::madvise_failure);
        return false;
    }
#endif

    return true;
}

// Revert settled pages at/above rows to committed anonymous memory and
// restore the retained bytes below rows from the file (truncation below the
// settle boundary). Failure results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::unsettle_(size_t rows) NOEXCEPT
{
    const auto bytes = to_width<Column>(rows);
    const auto begin = page_floor(bytes);
    const auto end = page_floor(to_width<Column>(settled_.load()));
    if (begin == end)
        return true;

    const auto address = std::next(memory_map_[Column], begin);
    if (mmap_unsettle(address, end - begin) == fail)
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

    if ((begin < bytes) && !pread_all(opened_[Column], address, bytes - begin,
        begin))
    {
        teardown_<Column>(error::fsync_failure);
        return false;
    }

    return true;
}

// Release cached pages of settled rows [from, to), page floored within the
// converted read-only mapping (its pages cannot be dirtied, and any pending
// settle write-back is synced by the release). Cost follows residency, so
// re-eviction is idempotent and free. The mapping is unaffected (a later
// read faults the page back from the file). Failure faults the store
// (advisory-class failures have no benign modes) but leaves it mapped, as
// the shared-lock caller cannot tear down under live accessors.
TEMPLATE
template <size_t Column>
bool CLASS::evict_(size_t from, size_t to) NOEXCEPT
{
    const auto begin = page_floor(to_width<Column>(from));
    const auto end = page_floor(to_width<Column>(to));
    if (begin == end)
        return true;

    if (mmap_evict(std::next(memory_map_[Column], begin), end - begin) == fail)
    {
        set_first_code(error::fsync_failure);
        return false;
    }

    return true;
}

// write throttle.
// ----------------------------------------------------------------------------
// private

// Delay the caller while staging exceeds its memory bound (write throttle).
// Signaled as the worker drains; a fault or worker stop releases the caller
// into the normal allocation path (which fails on fault/unload). The relieved
// state is lock-free for the common (unparked) case; notifiers synchronize on
// throttle_mutex_ so a parked caller cannot miss its signal.
TEMPLATE
void CLASS::throttle_() NOEXCEPT
{
    const auto relieved = [this]() NOEXCEPT
    {
        using namespace system;
        const auto rows = floored_subtract(logical_.load(), settled_.load());
        const auto debt = ceilinged_multiply(rows, stride);
        return (debt <= limit_) || !working_.load() || fault_.load();
    };

    if (!staged_ || relieved())
        return;

    std::unique_lock throttle_lock(throttle_mutex_);

    throttle_cv_.wait(throttle_lock, relieved);
}

// Wake throttled callers, synchronized so a parking caller cannot miss the
// signal between its relieved check and its wait.
TEMPLATE
void CLASS::signal_() NOEXCEPT
{
    std::unique_lock throttle_lock(throttle_mutex_);

    throttle_cv_.notify_all();
}

// settle scheduler (worker thread, drains completed writes to clean cache).
// ----------------------------------------------------------------------------
// private

// Pressure-paced draining (the windows model): drain intensity follows memory
// conditions, settled pages remain cached, writers delay only at the staging
// memory bound (write throttle), sustained stillness drains the residual (the
// lazy writer) so a quiescent map converges to fully settled.
TEMPLATE
void CLASS::body_run_() NOEXCEPT
{
    // Tiers derive from physical memory, pressure and compression occupancy
    // (a pressure precursor: the kernel compresses without raising level).
    const auto memory  = system_memory();
    const auto urgent  = memory / urgent_factor;
    const auto active  = memory / active_factor;
    const auto squeeze = memory / compress_factor;
    const auto scarce  = memory / sweep_factor;
    const auto chunk   = std::max(one, settle_chunk / stride);
    const auto sweep   = std::max(one, evict_chunk / stride);

    // Ticks without allocation before idle draining (settling is not writing,
    // so draining does not hold its own clock).
    auto mark = logical_.load();
    size_t still{};
    size_t evicted{};

    const auto backlog = [this]() NOEXCEPT
    {
        using namespace system;
        return ceilinged_multiply(floored_subtract(frontier_.load(),
            settled_.load()), stride);
    };

    while (tick_())
    {
        const auto top = logical_.load();
        still = (top == mark) ? std::min(add1(still), idle_seconds) : zero;
        mark = top;

        // Scarcity is read directly: clean cache is reclaimable, so the
        // kernel pressure level does not raise while free memory exhausts.
        if (system_free() < scarce)
            evict_next_(sweep, evicted);

        auto bytes = backlog();
        if (is_zero(bytes))
            continue;

        // Urgency drains continuously, activity/stillness one chunk per tick.
        const auto driven =
            (system_pressure() > one) ||
            (system_compressed() > squeeze) ||
            (bytes > urgent);

        if (!driven &&
            (bytes <= active) &&
            (still < idle_seconds))
            continue;

        do
        {
            if (!settle_next_(chunk))
                break;

            bytes = backlog();
        }
        while (working_.load() && driven && (bytes > active));
    }
}

// Settle up to chunk completed rows: write under the shared remap lock
// (completed extents are immutable, writers proceed), convert under a brief
// exclusive. Durability remains a snapshot property (no sync here).
TEMPLATE
bool CLASS::settle_next_(size_t chunk) NOEXCEPT
{
    size_t from{};
    size_t target{};

    {
        std::shared_lock map_lock(remap_mutex_);

        if (!loaded_.load() || fault_.load())
            return false;

        from = settled_.load();
        target = std::min(frontier_.load(),
            system::ceilinged_add(from, chunk));

        if (target <= from)
            return false;

        if (!settle_write_(from, target, sequence{}))
        {
            set_first_code(error::fsync_failure);
            return false;
        }
    }

    std::unique_lock map_lock(remap_mutex_);

    if (!loaded_.load())
        return false;

    // A raced settle boundary (flush) invalidates only this conversion.
    if (settled_.load() != from)
        return true;

    return settle_all_(target, sequence{});
}

// Evict up to chunk settled rows under memory scarcity: a wrapping sweep
// from the oldest offset (offset is write age in an append-only body, and
// re-read probability decays with it). Scarcity-driven release keeps the
// machine out of the exhausted-cache regime, where the kernel periodic
// sync walk (cost follows resident pages, not dirty) otherwise degrades
// fault service. A wrong eviction costs one cold read, so the sweep needs
// no precision. Runs under the shared remap lock (readers proceed).
TEMPLATE
bool CLASS::evict_next_(size_t chunk, size_t& cursor) NOEXCEPT
{
    std::shared_lock map_lock(remap_mutex_);

    if (!loaded_.load() || fault_.load())
        return false;

    // A truncated boundary self-heals here (the cursor clamps to a lap).
    const auto end = settled_.load();

    // Confine the lap to the recently settled tail: cache holds what was
    // just written (and what validation just read), while the body below
    // is long evicted, so a full-body lap sweeps unresident rows for most
    // of its length and the tail refills faster than the cursor returns.
    // A lap can hold no more than physical memory, as nothing else is
    // resident to evict.
    const auto span = system::greater(chunk, system_memory() / stride);
    const auto floor = (end > span) ? (end - span) : zero;
    const auto from = ((cursor > floor) && (cursor < end)) ? cursor : floor;
    const auto target = std::min(end, system::ceilinged_add(from, chunk));

    if (target <= from)
        return false;

    if (!evict_all_(from, target, sequence{}))
        return false;

    // Wrap at the lap end so the next sweep resumes at the tail floor.
    cursor = (target == end) ? zero : target;
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif // MANAGE_STAGING

#endif
