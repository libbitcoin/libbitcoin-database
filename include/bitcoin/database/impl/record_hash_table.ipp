/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/record_row.hpp>

namespace libbitcoin {
namespace database {

template <typename KeyType>
record_hash_table<KeyType>::record_hash_table(header_type& header,
    record_manager& manager)
  : header_(header), manager_(manager)
{
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated except in the order written.
template <typename KeyType>
typename record_hash_table<KeyType>::offset_type record_hash_table<KeyType>::store(
    const KeyType& key, write_function write)
{
    // Allocate and populate new unlinked record.
    record_row<KeyType, index_type> record(manager_);
    const auto index = record.create(key, write);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    create_mutex_.lock();

    // Link new record.next to current first record.
    record.link(read_bucket_value(key));

    // Link header to new record as the new first.
    link(key, index);

    create_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Return the array index of the new record (starts at key, not value).
    return index;
}

// Execute a writer against a key's buffer if the key is found.
// Return the array index of the found value (or not_found).
template <typename KeyType>
typename record_hash_table<KeyType>::offset_type record_hash_table<KeyType>::update(
    const KeyType& key, write_function write)
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != not_found)
    {
        const record_row<KeyType, index_type> item(manager_, current);

        // Found, update data and return index.
        if (item.compare(key))
        {
            const auto memory = item.data();
            auto serial = make_unsafe_serializer(memory->buffer());
            write(serial);
            return current;
        }

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_index();
        ///////////////////////////////////////////////////////////////////////
    }

    return not_found;
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType>
typename record_hash_table<KeyType>::offset_type record_hash_table<KeyType>::offset(
    const KeyType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const record_row<KeyType, index_type> item(manager_, current);

        // Found, return index.
        if (item.compare(key))
            return item.offset();

        const auto previous = current;
        current = item.next_index();
        BITCOIN_ASSERT(previous != current);
    }

    return not_found;
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType>
memory_ptr record_hash_table<KeyType>::find(const KeyType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != not_found)
    {
        const record_row<KeyType, index_type> item(manager_, current);

        // Found, return pointer.
        if (item.compare(key))
            return item.data();

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_index();
        ///////////////////////////////////////////////////////////////////////
    }

    return nullptr;
}

// Unlink is not safe for concurrent write.
// This is limited to unlinking the first of multiple matching key values.
template <typename KeyType>
bool record_hash_table<KeyType>::unlink(const KeyType& key)
{
    // Find start item...
    auto previous = read_bucket_value(key);
    const record_row<KeyType, index_type> begin_item(manager_, previous);

    // If start item has the key then unlink from buckets.
    if (begin_item.compare(key))
    {
        //*********************************************************************
        const auto next = begin_item.next_index();
        //*********************************************************************

        link(key, next);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    update_mutex_.lock_shared();
    auto current = begin_item.next_index();
    update_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Iterate through list...
    while (current != not_found)
    {
        const record_row<KeyType, index_type> item(manager_, current);

        // Found, unlink current item from previous.
        if (item.compare(key))
        {
            record_row<KeyType, index_type> previous_item(manager_, previous);

            // Critical Section
            ///////////////////////////////////////////////////////////////////
            update_mutex_.lock_upgrade();
            const auto next = item.next_index();
            update_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            previous_item.write_next_index(next);
            update_mutex_.unlock();
            ///////////////////////////////////////////////////////////////////
            return true;
        }

        previous = current;

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_index();
        ///////////////////////////////////////////////////////////////////////
    }

    return false;
}

// private
template <typename KeyType>
typename record_hash_table<KeyType>::index_type record_hash_table<KeyType>::bucket_index(
    const KeyType& key) const
{
    return remainder(key, header_.buckets());
}

// private
template <typename KeyType>
typename record_hash_table<KeyType>::offset_type record_hash_table<KeyType>::read_bucket_value(
    const KeyType& key) const
{
    return header_.read(bucket_index(key));
}

// private
template <typename KeyType>
void record_hash_table<KeyType>::link(const KeyType& key, offset_type begin)
{
    header_.write(bucket_index(key), begin);
}

} // namespace database
} // namespace libbitcoin

#endif
