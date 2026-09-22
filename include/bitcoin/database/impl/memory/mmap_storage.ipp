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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_STORAGE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_STORAGE_IPP

#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>

namespace libbitcoin {
namespace database {

// Interface.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return error_.load();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return space_.load();
}

TEMPLATE
const std::filesystem::path& CLASS::file() const NOEXCEPT
{
    return filenames_.front();
}

TEMPLATE
code CLASS::create() const NOEXCEPT
{
    for (const auto& descriptor: opened_)
        if (descriptor != file::invalid)
            return error::create_open;

    for (const auto& name: filenames_)
        if (const auto ec = file::create_file_ex(name))
            return ec;

    return error::success;
}

TEMPLATE
code CLASS::open() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    for (const auto& descriptor: opened_)
        if (descriptor != file::invalid)
            return error::open_open;

    // Windows doesn't use madvise, instead infers map access from file open.
    for (size_t index{}; index < columns; ++index)
        if (const auto ec = file::open_ex(opened_.at(index),
            filenames_.at(index), random_, access_))
            return ec;

    // logical_ is the shared row count, derived from column 0's byte size.
    size_t bytes{};
    if (const auto ec = file::size_ex(bytes, opened_.front()))
        return ec;

    // The file as opened is its own provisioning (extent tracks the file).
    logical_.store(logical_rows(bytes));
    file_.store(logical_rows(bytes));
    return error::success;
}

TEMPLATE
code CLASS::close() NOEXCEPT
{
    std::unique_lock map_lock(remap_mutex_);
    std::unique_lock field_lock(field_mutex_);

    if (loaded_.load())
        return error::close_loaded;

    if (opened_.front() == file::invalid)
        return error::success;

    logical_.store(zero);
    for (auto& descriptor: opened_)
    {
        if (descriptor != file::invalid)
        {
            const auto current = descriptor;
            descriptor = file::invalid;
            if (const auto ec = file::close_ex(current))
                return ec;
        }
    }

    return error::success;
}

// map, flush, unmap.
// ----------------------------------------------------------------------------
// Each accessor holds a shared lock on remap_mutex_ (read/write access to map).
// Exclusive lock on remap_mutex_ ensures there are open accessor objects,
// which allows for safely remapping the memory map.

TEMPLATE
code CLASS::load() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (loaded_.load())
        {
            remap_mutex_.unlock();
            return error::load_loaded;
        }

        // Updates fields.
        if (!map_all_(sequence{}))
        {
            remap_mutex_.unlock();
            return error::load_failure;
        }

        remap_mutex_.unlock();
        worker_start_();
        return error::success;
    }

    return error::load_locked;
}

// Suspend writes before calling.
TEMPLATE
code CLASS::reload() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_.load())
        {
            remap_mutex_.unlock();
            return error::reload_unloaded;
        }

#if defined(MANAGE_STAGING)
        // The store reload transactor implies quiescent writers, so remaining
        // extents are orphans of abandoned writes, stalling the frontier (in
        // any table, not just those that faulted). Discard to unjam settling.
        discard_();
#endif

        // Allow resume from disk full.
        set_disk_space(zero);

        remap_mutex_.unlock();
        return error::success;
    }

    // Locked by reader(s), as write suspension is a precondition.
    return error::reload_locked;
}

// Suspend writes before calling.
TEMPLATE
code CLASS::flush() NOEXCEPT
{
    // The suspend-writes contract holds logical_ stable across both phases.
    size_t rows{};

    {
        // Prevent unload, resize, remap.
        std::shared_lock map_lock(remap_mutex_);

        if (!loaded_.load())
            return error::flush_unloaded;

        rows = logical_.load();

        // Reads fields and the memory map.
        if (!flush_all_(rows, sequence{}))
            return error::flush_failure;
    }

#if defined(MANAGE_STAGING)
    // Convert flushed rows to read-only file mappings, releasing their
    // anonymous pages to clean file cache (excludes accessors, briefly).
    if (staged_)
    {
        std::unique_lock map_lock(remap_mutex_);

        if (!loaded_.load())
            return error::flush_unloaded;

        // The suspend-writes contract implies remaining extents are complete
        // or abandoned (as reload), so the ring is discarded to reconcile the
        // frontier, which settling to the flushed top would otherwise pass.
        discard_();

        if (!settle_all_(rows, sequence{}))
            return error::flush_failure;
    }
#endif

    return error::success;
}

// Suspend writes before calling.
TEMPLATE
code CLASS::unload() NOEXCEPT
{
    worker_stop_();
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_.load())
        {
            remap_mutex_.unlock();
            return error::success;
        }

        // Updates fields.
        if (!unmap_all_(sequence{}))
        {
            remap_mutex_.unlock();
            return error::unload_failure;
        }

        remap_mutex_.unlock();
        return error::success;
    }

    return error::unload_locked;
}

// Suspend writes before calling.
TEMPLATE
code CLASS::shrink() NOEXCEPT
{
    worker_stop_();
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_.load())
        {
            remap_mutex_.unlock();
            return error::shrink_unloaded;
        }

        // Updates fields.
        if (!unmap_all_(sequence{}))
        {
            remap_mutex_.unlock();
            return error::shrink_unload_failure;
        }

        // Updates fields.
        if (!map_all_(sequence{}))
        {
            remap_mutex_.unlock();
            return error::shrink_load_failure;
        }

        remap_mutex_.unlock();
        worker_start_();
        return error::success;
    }

    return error::shrink_locked;
}

// Used to copy headers in snapshot (scalar only).
TEMPLATE
code CLASS::dump(const std::filesystem::path& path) const NOEXCEPT
{
    BC_ASSERT(is_one(columns));
    const auto ptr = get();
    if (!ptr)
        return error::unloaded_file;

    return file::create_file_ex(path, ptr.begin(), ptr.size());
}

// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::size() const NOEXCEPT
{
    return logical_.load();
}

TEMPLATE
size_t CLASS::capacity() const NOEXCEPT
{
    return capacity_.load();
}

TEMPLATE
bool CLASS::truncate(size_t count) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (count > logical_.load())
        return false;

#if defined(MANAGE_STAGING)
    // Truncation below the settle boundary reverts settled rows to anonymous
    // memory, as append-only bodies may re-append after reorganization.
    if (staged_ && loaded_.load() && (count < settled_.load()))
    {
        std::unique_lock remap_lock(remap_mutex_);

        if (!unsettle_all_(count, sequence{}))
            return false;
    }

    trim_(count);
#endif

    logical_.store(count);
    check_invariants_();
    return true;
}

TEMPLATE
bool CLASS::expand(size_t count) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_.load() || !loaded_.load())
        return false;

    if (count <= logical_.load())
        return true;

    if (count > capacity_.load())
    {
        std::unique_lock remap_lock(remap_mutex_);
        if (!grow_(count))
            return false;
    }

    // Raise to at least count (concurrent claims may already exceed it).
    for (auto current = logical_.load(); current < count;)
        if (logical_.compare_exchange_weak(current, count))
            break;

    return true;
}

TEMPLATE
bool CLASS::reserve(size_t count) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    using namespace system;
    if (fault_.load() || !loaded_.load() ||
        is_add_overflow(logical_.load(), count))
        return false;

    const auto end = logical_.load() + count;
    if (end > capacity_.load())
    {
        std::unique_lock remap_lock(remap_mutex_);
        if (!grow_(end))
            return false;
    }

    // Same as allocate except logical does not change.
    return true;
}

// The slow path waits until all access pointers are destructed. Will deadlock
// if any access pointer is waiting on allocation. Lock safety requires that
// access pointers are short-lived and do not block on allocation. Relies on
// the suspend-writes contract of shrinking transitions (truncate/unload/
// shrink/close), as the fast path does not serialize against them.
TEMPLATE
size_t CLASS::allocate(size_t count) NOEXCEPT
{
    using namespace system;

#if defined(MANAGE_STAGING)
    // Nothing is held here, so parking cannot deadlock (as with remap waits).
    throttle_();

    // Staged claims serialize with extent recording (record_ locks on every
    // claim regardless, so this adds no contention): a claim never exists
    // outside the ring, so the frontier can never pass an unwritten extent.
    if (staged_)
    {
        while (true)
        {
            if (fault_.load() || !loaded_.load())
                return storage::eof;

            const auto start = record_(count);
            if (start != storage::eof)
                return start;

            // Slow path: serialize capacity growth (at most one grower).
            std::unique_lock field_lock(field_mutex_);

            const auto logical = logical_.load();
            if (is_add_overflow(logical, count))
                return storage::eof;

            const auto end = logical + count;
            if (end > capacity_.load())
            {
                std::unique_lock remap_lock(remap_mutex_);
                if (!grow_(end))
                    return storage::eof;
            }
        }
    }
#endif // MANAGE_STAGING

    // Fast path: claim rows within published capacity (no locks). A failed
    // exchange implies another claim succeeded, so every retry is progress.
    auto start = logical_.load();
    while (true)
    {
        if (fault_.load() || !loaded_.load() || is_add_overflow(start, count))
            return storage::eof;

        if ((start + count) > capacity_.load())
            break;

        if (logical_.compare_exchange_weak(start, start + count))
            return start;
    }

    // Slow path: serialize capacity growth (at most one grower). Fast paths
    // continue claiming under the old capacity; the growth target covers them.
    std::unique_lock field_lock(field_mutex_);

    start = logical_.load();
    while (true)
    {
        if (fault_.load() || !loaded_.load() || is_add_overflow(start, count))
            return storage::eof;

        const auto end = start + count;
        if (end <= capacity_.load())
        {
            if (logical_.compare_exchange_weak(start, end))
                return start;

            continue;
        }

        // TODO: Could loop over a try lock here and log deadlock warning.
        std::unique_lock remap_lock(remap_mutex_);

        // Disk full condition leaves store in valid state despite eof return.
        if (!grow_(end))
            return storage::eof;
    }
}

// Backfilled allocation (head creation). A managed head writes the fill to
// its file and maps it released, so the fill is never memory-resident; a
// native mapping fills through memory (file-backed, kernel writeback).
TEMPLATE
size_t CLASS::allocate(size_t count, uint8_t backfill) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (managed_ && !shared_.load())
        return allocate_filled_(count, backfill);
#endif

    const auto start = allocate(count);
    if (start == storage::eof)
        return start;

    // Growth provisions above the allocation, so fill through capacity.
    const auto offset = to_width<zero>(start);
    const auto ptr = get_capacity(offset);
    if (!ptr)
        return storage::eof;

    const auto size = system::possible_narrow_sign_cast<size_t>(ptr.size());
    prepare(offset, size);
    std::fill_n(ptr.data(), size, backfill);
    mark(offset, size);
    return start;
}

} // namespace database
} // namespace libbitcoin

#endif
