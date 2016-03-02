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
#ifndef LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_IPP

#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include "../impl/record_row.ipp"
#include "../impl/remainder.ipp"

namespace libbitcoin {
namespace database {

template <typename HashType>
record_hash_table<HashType>::record_hash_table(
    record_hash_table_header& header, record_manager& manager)
  : header_(header), manager_(manager)
{
}

template <typename HashType>
void record_hash_table<HashType>::store(const HashType& key,
    const write_function write)
{
    // Store current bucket value.
    const auto old_begin = read_bucket_value(key);
    record_row<HashType> item(manager_, 0);
    const auto new_begin = item.create(key, old_begin);
    const auto memory = item.data();
    write(memory->buffer());

    // Link record to header.
    link(key, new_begin);
}

template <typename HashType>
const memory::ptr record_hash_table<HashType>::find(const HashType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const record_row<HashType> item(manager_, current);

        // Found, return data.
        if (item.compare(key))
            return item.data();

        const auto previous = current;
        current = item.next_index();

        // This may otherwise produce an infinite loop here.
        // It indicates that a write operation has interceded.
        // So we must return gracefully vs. looping forever.
        if (previous == current)
            return nullptr;
    }

    return nullptr;
}

template <typename HashType>
bool record_hash_table<HashType>::unlink(const HashType& key)
{
    // Find start item...
    const auto begin = read_bucket_value(key);
    const record_row<HashType> begin_item(manager_, begin);

    // If start item has the key then unlink from buckets.
    if (begin_item.compare(key))
    {
        link(key, begin_item.next_index());
        return true;
    }

    // Continue on...
    auto previous = begin;
    auto current = begin_item.next_index();

    // Iterate through list...
    while (current != header_.empty)
    {
        const record_row<HashType> item(manager_, current);

        // Found, unlink current item from previous.
        if (item.compare(key))
        {
            release(item, previous);
            return true;
        }

        previous = current;
        current = item.next_index();

        // This may otherwise produce an infinite loop here.
        // It indicates that a write operation has interceded.
        // So we must return gracefully vs. looping forever.
        if (previous == current)
            return false;
    }

    return false;
}

template <typename HashType>
array_index record_hash_table<HashType>::bucket_index(
    const HashType& key) const
{
    const auto bucket = remainder(key, header_.size());
    BITCOIN_ASSERT(bucket < header_.size());
    return bucket;
}

template <typename HashType>
array_index record_hash_table<HashType>::read_bucket_value(
    const HashType& key) const
{
    auto value = header_.read(bucket_index(key));
    BITCOIN_ASSERT(sizeof(value) == sizeof(array_index));
    return value;
}

template <typename HashType>
void record_hash_table<HashType>::link(const HashType& key,
    const array_index begin)
{
    header_.write(bucket_index(key), begin);
}

template <typename HashType>
template <typename ListItem>
void record_hash_table<HashType>::release(const ListItem& item,
    const file_offset previous)
{
    ListItem previous_item(manager_, previous);
    previous_item.write_next_index(item.next_index());
}

} // namespace database
} // namespace libbitcoin

#endif
