/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/primitives/record_list.hpp>

#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

record_list::record_list(record_manager& manager)
  : manager_(manager)
{
    static_assert(sizeof(array_index) == sizeof(uint32_t),
        "array_index incorrect size");
}

array_index record_list::create()
{
    // Insert new record with null next value.
    return insert(empty);
}

array_index record_list::insert(array_index next)
{
    // Create new record.
    auto index = manager_.new_records(1);
    const auto memory = manager_.get(index);
    auto serial = make_serializer(REMAP_ADDRESS(memory));

    // Write next index at first 4 bytes of the record preceding the insert.
    //*************************************************************************
    serial.write_4_bytes_little_endian(next);
    //*************************************************************************

    // Return the position of the new record.
    return index;
}

array_index record_list::next(array_index index) const
{
    const auto memory = manager_.get(index);
    //*************************************************************************
    return from_little_endian_unsafe<array_index>(REMAP_ADDRESS(memory));
    //*************************************************************************
}

const memory_ptr record_list::get(array_index index) const
{
    auto memory = manager_.get(index);
    REMAP_INCREMENT(memory, sizeof(array_index));
    return memory;
}

} // namespace database
} // namespace libbitcoin
