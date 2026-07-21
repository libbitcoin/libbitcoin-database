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
memory CLASS::get_filled(size_t offset, size_t size,
    uint8_t backfill) NOEXCEPT
{
    // This is basically allocate(...), backfilled for use with a table head.
    {
        std::unique_lock field_lock(field_mutex_);

        if (fault_.load() || !loaded_.load() ||
            system::is_add_overflow(offset, size))
            return {};

        const auto end = std::max(logical_.load(), offset + size);
        if (end > capacity_.load())
        {
            const auto extended = to_capacity(end);

            // TODO: Could loop over a try lock here and log deadlock warning.
            std::unique_lock remap_lock(remap_mutex_);

            // Disk full condition leaves store in valid state despite null.
            if (!remap_all_(extended, sequence{}))
                return {};

            // Fill new capacity as offset may not be at end due to expansion.
            auto data = memory_map_.front();
            const auto logical = to_width<zero>(logical_.load());
            const auto capacity = to_width<zero>(capacity_.load());
            const auto start = std::next(data, logical);
            std::fill_n(start, capacity - logical, backfill);
        }

        // Raise to at least end (concurrent claims may already exceed it).
        for (auto current = logical_.load(); current < end;)
            if (logical_.compare_exchange_weak(current, end))
                break;
    }

    return get(offset);
}

TEMPLATE
memory CLASS::get_capacity(size_t offset) const NOEXCEPT
{
    const auto allocated = to_width<zero>(capacity());

    memory out{ remap_mutex_ };

    if (!loaded_.load())
        return {};

    auto data = memory_map_.front();
    out.assign(std::next(data, offset), std::next(data, allocated));
    return out;
}

TEMPLATE
memory::iterator CLASS::get_raw(size_t offset) const NOEXCEPT
{
    // Pointer otherwise unguarded, not remap safe (use for fixed table heads).
    return get_raw_at(zero, offset);
}

TEMPLATE
memory::iterator CLASS::get_raw_at(size_t column, size_t offset) const NOEXCEPT
{
    // get_raw not used for variably-sized heads, so should always be bounded.
    BC_ASSERT(offset < (size() * widths.at(column)));

    // Pointer otherwise unguarded, not remap safe (use for nopmaps columns).
    return std::next(memory_map_.at(column), offset);
}

TEMPLATE
memory CLASS::get(size_t offset) const NOEXCEPT
{
    return get_at(zero, offset);
}

TEMPLATE
memory CLASS::get_at(size_t column, size_t offset) const NOEXCEPT
{
    if (column >= columns)
        return {};

    // size() is a lock-free atomic read.
    const auto allocated = size() * widths.at(column);

    // Takes a shared lock on remap_mutex_ until destruct, blocking remap.
    memory out{ remap_mutex_ };

    // loaded_ update is precluded by above lock.
    if (!loaded_.load())
        return {};

    // With offset > size the assignment is negative (stream is exhausted).
    auto data = memory_map_.at(column);
    out.assign(std::next(data, offset), std::next(data, allocated));
    return out;
}

} // namespace database
} // namespace libbitcoin

#endif
