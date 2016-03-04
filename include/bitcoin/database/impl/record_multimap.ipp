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
#ifndef LIBBITCOIN_DATABASE_RECORD_MULTIMAP_IPP
#define LIBBITCOIN_DATABASE_RECORD_MULTIMAP_IPP

#include <string>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

template <typename HashType>
record_multimap<HashType>::record_multimap(record_hash_table_type& map,
    record_list& linked_rows)
  : map_(map), records_(linked_rows)
{
}

template <typename HashType>
array_index record_multimap<HashType>::lookup(const HashType& key) const
{
    const auto memory = map_.find(key);

    if (!memory)
        return records_.empty;

    const auto start_info = REMAP_ADDRESS(memory);
    const auto first = from_little_endian_unsafe<array_index>(start_info);
    return first;
}

template <typename HashType>
void record_multimap<HashType>::add_row(const HashType& key,
    write_function write)
{
    const auto memory = map_.find(key);

    if (!memory)
    {
        create_new(key, write);
        return;
    }

    const auto start_info = REMAP_ADDRESS(memory);
    add_to_list(start_info, write);
}

template <typename HashType>
void record_multimap<HashType>::delete_last_row(const HashType& key)
{
    const auto memory = map_.find(key);
    BITCOIN_ASSERT_MSG(memory, "The row to delete was not found.");

    const auto start_info = REMAP_ADDRESS(memory);
    const auto old_begin = from_little_endian_unsafe<array_index>(start_info);

    BITCOIN_ASSERT(old_begin != records_.empty);
    const auto new_begin = records_.next(old_begin);

    if (new_begin == records_.empty)
    {
        DEBUG_ONLY(bool success =) map_.unlink(key);
        BITCOIN_ASSERT(success);
        return;
    }

    // MUST BE ATOMIC
    auto serial = make_serializer(start_info);
    serial.write_4_bytes_little_endian(new_begin);
}

template <typename HashType>
void record_multimap<HashType>::add_to_list(uint8_t* start_info,
    write_function write)
{
    const auto old_begin = from_little_endian_unsafe<array_index>(start_info);
    const auto new_begin = records_.insert(old_begin);
    const auto memory = records_.get(new_begin);
    write(REMAP_ADDRESS(memory));
    auto serial = make_serializer(start_info);

    // MUST BE ATOMIC
    serial.write_4_bytes_little_endian(new_begin);
}

template <typename HashType>
void record_multimap<HashType>::create_new(const HashType& key,
    write_function write)
{
    const auto first = records_.create();
    const auto memory = records_.get(first);
    write(REMAP_ADDRESS(memory));
    const auto write_start_info = [first](uint8_t* data)
    {
        // MUST BE ATOMIC
        auto serial = make_serializer(data);
        serial.write_4_bytes_little_endian(first);
    };
    map_.store(key, write_start_info);
}

} // namespace database
} // namespace libbitcoin

#endif
