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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_DISPATCH_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_DISPATCH_IPP

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <utility>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>

namespace libbitcoin {
namespace database {

// Dispatch.
// ----------------------------------------------------------------------------

TEMPLATE
template <size_t Column, if_lesser<Column, sizeof...(Widths)>>
memory_ptr CLASS::set_column(size_t offset, size_t size,
    uint8_t backfill) NOEXCEPT
{
    {
        std::unique_lock field_lock(field_mutex_);

        if (fault_ || !loaded_ || system::is_add_overflow(offset, size))
            return {};

        const auto end = std::max(logical_, offset + size);
        if (end > capacity_rows(capacity_))
        {
            const auto capacity = to_capacity(end);

            // TODO: Could loop over a try lock here and log deadlock warning.
            std::unique_lock remap_lock(remap_mutex_);

            // Disk full condition leaves store in valid state despite null.
            if (!remap_all_(capacity, sequence{}))
                return {};

            // Fill new capacity as offset may not be at end due to expansion.
            const auto first = to_width<Column>(logical_);
            std::fill_n(
                std::next(std::get<Column>(memory_map_), first),
                std::get<Column>(capacity_) - first, backfill);
        }

        logical_ = end;
    }

    return get_column<Column>(offset);
}

TEMPLATE
template <size_t Column, if_lesser<Column, sizeof...(Widths)>>
memory_ptr CLASS::get_column(size_t offset) const NOEXCEPT
{
    // Obtaining size before access prevents mutual mutex wait (deadlock).
    // The store could remap between here and next line, but capacity only
    // increases. Close zeroizes capacity but file must be unloaded to do so.
    // Truncate can reduce logical, but capacity is not affected. It is always
    // safe to write past current logical within current capacity.
    const auto allocated = to_width<Column>(size());

    // Takes a shared lock on remap_mutex_ until destruct, blocking remap.
    const auto ptr = std::make_shared<access>(remap_mutex_);

    // loaded_ update is precluded by remap_mutex_, making this read atomic.
    if (!loaded_ || is_null(ptr))
        return nullptr;

    // With offset > size the assignment is negative (stream is exhausted).
    ptr->assign(
        std::next(std::get<Column>(memory_map_), offset),
        std::next(std::get<Column>(memory_map_), allocated));

    return ptr;
}

TEMPLATE
template <size_t Column, if_lesser<Column, sizeof...(Widths)>>
memory_ptr CLASS::capacity_column(size_t offset) const NOEXCEPT
{
    // Same as get() but limited by capacity() vs. size().
    const auto allocated = std::get<Column>(capacity_);
    const auto ptr = std::make_shared<access>(remap_mutex_);
    if (!loaded_ || is_null(ptr))
        return nullptr;

    ptr->assign(
        std::next(std::get<Column>(memory_map_), offset),
        std::next(std::get<Column>(memory_map_), allocated));

    return ptr;
}

TEMPLATE
template <size_t Column, if_lesser<Column, sizeof...(Widths)>>
memory::iterator CLASS::raw_column(size_t offset) const NOEXCEPT
{
    // Pointer is otherwise unguarded, not remap safe (use for table heads).
    if (offset > to_width<Column>(size()))
        return nullptr;

    return std::next(std::get<Column>(memory_map_), offset);
}

} // namespace database
} // namespace libbitcoin

#endif
