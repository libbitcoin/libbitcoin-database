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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_STORAGE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_STORAGE_IPP

#include <filesystem>
#include <mutex>
#include <utility>
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
            filenames_.at(index), random_))
            return ec;

    // logical_ is the shared row count, derived from column 0's byte size.
    size_t bytes{};
    if (const auto ec = file::size_ex(bytes, opened_.front()))
        return ec;

    logical_ = logical_rows(bytes);
    return error::success;
}

TEMPLATE
code CLASS::close() NOEXCEPT
{
    std::unique_lock map_lock(remap_mutex_);
    std::unique_lock field_lock(field_mutex_);

    if (loaded_)
        return error::close_loaded;

    if (opened_.front() == file::invalid)
        return error::success;

    logical_ = zero;
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
        if (loaded_)
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
        if (!loaded_)
        {
            remap_mutex_.unlock();
            return error::reload_unloaded;
        }

        // Allow resume from disk full.
        set_disk_space(zero);

        remap_mutex_.unlock();
        return error::success;
    }

    return error::reload_locked;
}

// Suspend writes before calling.
TEMPLATE
code CLASS::flush() NOEXCEPT
{
    // Prevent unload, resize, remap.
    std::shared_lock map_lock(remap_mutex_);
    std::shared_lock field_lock(field_mutex_);

    if (!loaded_)
        return error::flush_unloaded;

    // Reads fields and the memory map.
    return flush_all_(sequence{}) ? error::success : error::flush_failure;
}

// Suspend writes before calling.
TEMPLATE
code CLASS::unload() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_)
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
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_)
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

    return file::create_file_ex(path, ptr->begin(), ptr->size());
}

// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return logical_;
}

TEMPLATE
size_t CLASS::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return capacity_rows(capacity_);
}

TEMPLATE
bool CLASS::truncate(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (size > logical_)
        return false;

    logical_ = size;
    return true;
}

TEMPLATE
bool CLASS::expand(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_ || !loaded_)
        return false;

    if (size <= logical_)
        return true;

    // Column 0 capacity (bytes) transposed to rows for the row-space compare.
    if (size > capacity_rows(capacity_))
    {
        const auto capacity = to_capacity(size);
        std::unique_lock remap_lock(remap_mutex_);
        if (!remap_all_(capacity, sequence{}))
            return false;
    }

    logical_ = size;
    return true;
}

TEMPLATE
bool CLASS::reserve(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_ || !loaded_ || system::is_add_overflow(logical_, chunk))
        return false;

    const auto end = logical_ + chunk;
    if (end > capacity_rows(capacity_))
    {
        const auto capacity = to_capacity(end);
        std::unique_lock remap_lock(remap_mutex_);
        if (!remap_all_(capacity, sequence{}))
            return false;
    }

    // Same as allocate except logical does not change.
    return true;
}

// Waits until all access pointers are destructed. Will deadlock if any access
// pointer is waiting on allocation. Lock safety requires that access pointers
// are short-lived and do not block on allocation.
TEMPLATE
size_t CLASS::allocate(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_ || !loaded_ || system::is_add_overflow(logical_, chunk))
        return storage::eof;

    const auto current = capacity_rows(capacity_);
    auto end = logical_ + chunk;
    if (end > current)
    {
        const auto capacity = to_capacity(end);

        // TODO: Could loop over a try lock here and log deadlock warning.
        std::unique_lock remap_lock(remap_mutex_);

        // Disk full condition leaves store in valid state despite eof return.
        if (!remap_all_(capacity, sequence{}))
            return storage::eof;
    }

    std::swap(logical_, end);
    return end;
}

} // namespace database
} // namespace libbitcoin

#endif
