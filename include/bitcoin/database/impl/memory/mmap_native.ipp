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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_NATIVE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_NATIVE_IPP

#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mman.hpp>
#include <bitcoin/database/memory/utilities.hpp>

#if defined(HAVE_MSC)

namespace libbitcoin {
namespace database {

// native backend (file-backed mapping), not thread safe.
// ----------------------------------------------------------------------------
// private

// Never results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::flush_(size_t rows) NOEXCEPT
{
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    const auto size = to_width<Column>(rows);
    const auto success =
           (::msync(memory_map_[Column], size, MS_SYNC) != fail)
        && (::fsync(opened_[Column]) != fail);

    if (!success)
        set_first_code(error::fsync_failure);

    return success;
}

// Mapping failure results in unmapped.
// Mapping has no effect on logical size, always maps max(logical, min) size.
TEMPLATE
template <size_t Column>
bool CLASS::map_() NOEXCEPT
{
    // Cannot map empty file, and want minimum capacity, so expand as required.
    // The classic mapping is file-backed, so commitment is provisioning.
    // disk_full: space is set but no code is set with false return.
    const auto size = to_provision();
    if (!resize_<Column>(size))
        return false;

    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, to_width<Column>(size), PROT_READ | PROT_WRITE,
            MAP_SHARED, opened_[Column], 0));

    return finalize_<Column>();
}

// Always results in unmapped, trims to logical (can be zero).
TEMPLATE
template <size_t Column>
bool CLASS::unmap_(size_t size) NOEXCEPT
{
    const auto logical = to_width<Column>(logical_.load());

    // Windows cannot resize a mapped file.
    // msync requires the live mapping, ftruncate requires it gone.
    const auto synced =
           (::msync(memory_map_[Column], logical, MS_SYNC) != fail);

    // Order ensures release in case of sync failure.
    const auto success = release_<Column>(size) && synced
        && (::ftruncate(opened_[Column], logical) != fail)
        && (::fsync(opened_[Column]) != fail);

    loaded_.store(false);

    if (!success)
        set_first_code(error::munmap_failure);

    return success;
}

// Remap failure results in unmapped.
// Remapping has no effect on logical size, sets map_/capacity_.
TEMPLATE
template <size_t Column>
bool CLASS::remap_(size_t size, bool final) NOEXCEPT
{
    BC_ASSERT(size >= logical_.load());

    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = minimum_;

    if (!resize_<Column>(size, final))
        return false;

    // mman-win32 mremap hack (umap/map) requires flags and file descriptor.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap_(memory_map_[Column], to_width<Column>(capacity_.load()),
            to_width<Column>(size), PROT_READ | PROT_WRITE, MAP_SHARED,
            opened_[Column]));

    return finalize_<Column>();
}

// Always results in unmapped, file is unchanged.
TEMPLATE
template <size_t Column>
bool CLASS::release_(size_t size) NOEXCEPT
{
    const auto success =
        ::munmap(memory_map_[Column], to_width<Column>(size)) != fail;

    if (!success)
        set_first_code(error::munmap_failure);

    // loaded_ is caller-owned: unmap_ publishes unloaded, remap_ remains
    // loaded across replacement (lock-free allocate guards must not observe
    // a transient unload).
    memory_map_[Column] = {};
    return success;
}

// Finalize failure results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::finalize_() NOEXCEPT
{
    if (memory_map_[Column] == MAP_FAILED)
    {
        loaded_.store(false);
        memory_map_[Column] = {};

        // mmap or mremap failure (not mapped).
        set_first_code(error::mmap_failure);
        return false;
    }

    loaded_.store(true);
    return true;
}

// working set scan (worker thread).
// ----------------------------------------------------------------------------
// private

// Heads assert residency, bodies lead the trim. Both are advisory: failure
// alters nothing and is not a fault (unlike the staged eviction primitives,
// which own persistence).
TEMPLATE
void CLASS::scan_run_() NOEXCEPT
{
    using namespace system;
    const auto page = page_size();
    if (is_zero(page))
        return;

    // Idempotent across instances (process-wide, same values each call).
    const auto memory = system_memory();
    /* int */ ::working_floor((memory / 4) * 3, memory);

    const auto span = std::max(one, evict_chunk / stride);
    size_t touched{};
    size_t unlocked{};

    while (tick_())
    {
        // Steering is only useful against a trim, and a trim only happens
        // under contention, so at plenty the tick is a counter test. This
        // also bounds the idle cost to the wait itself.
        if (system_available() >= (system_memory() / sweep_factor))
            continue;

        std::shared_lock map_lock(remap_mutex_);
        if (!loaded_.load() || fault_.load())
            continue;

        if (staged_)
        {
            // Unlock a wrapping lap of the oldest rows (offset is write age
            // in an append-only body, and re-read probability decays with
            // it), so the trim takes these before the head set.
            const auto rows = logical_.load();
            const auto from = (unlocked < rows) ? unlocked : zero;
            const auto to = std::min(rows, ceilinged_add(from, span));
            if (to <= from)
                continue;

            const auto at = to_width<zero>(from);
            /* bool */ ::munlock(std::next(memory_map_[zero], at),
                to_width<zero>(to) - at);

            unlocked = (to == rows) ? zero : to;
        }
        else
        {
            // Revisit every head page each touch_seconds regardless of
            // instance size. A read sets the access bit without dirtying
            // the page; volatile prevents elision.
            const auto pages = to_width<zero>(logical_.load()) / page;
            const volatile auto* map = memory_map_[zero];
            auto budget = ceilinged_divide(pages, touch_seconds);

            while (!is_zero(pages) && !is_zero(budget))
            {
                if (touched >= pages)
                    touched = zero;

                const auto at = touched * page;
                const auto count = std::min(
                    { touch_span, pages - touched, budget });

                for (size_t index{}; index < count; ++index)
                    (void)map[at + index * page];

                touched += count;
                budget = floored_subtract(budget, count);
            }
        }
    }
}

} // namespace database
} // namespace libbitcoin

#endif // HAVE_MSC

#endif
