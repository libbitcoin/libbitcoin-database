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
#include <bitcoin/database/hash_table/record_list.hpp>

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
    // Insert new record with empty next value.
    return insert(empty);
}

array_index record_list::insert(array_index next)
{
    // Create new record.
    auto index = manager_.new_records(1);
    const auto memory = manager_.get(index);

    // Write next value at first 4 bytes of record.
    auto serial = make_serializer(ADDRESS(memory));

    // MUST BE ATOMIC
    serial.write_4_bytes_little_endian(next);
    return index;
}

array_index record_list::next(array_index index) const
{
    const auto memory = manager_.get(index);
    return from_little_endian_unsafe<array_index>(ADDRESS(memory));
}

const memory_ptr record_list::get(array_index index) const
{
    auto memory = manager_.get(index);
    INCREMENT(memory, sizeof(array_index));
    return memory;
}

} // namespace database
} // namespace libbitcoin
