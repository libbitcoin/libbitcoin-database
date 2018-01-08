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
#ifndef LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/hash_table_iterator.hpp>
#include <bitcoin/database/primitives/slab_row.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename KeyType, typename IndexType, typename LinkType>
slab_hash_table<KeyType, IndexType, LinkType>::slab_hash_table(storage& file,
    IndexType buckets)
  : header_(file, buckets),
    manager_(file, header::size(buckets))
{
}

template <typename KeyType, typename IndexType, typename LinkType>
bool slab_hash_table<KeyType, IndexType, LinkType>::create()
{
    return header_.create() && manager_.create();
}

template <typename KeyType, typename IndexType, typename LinkType>
bool slab_hash_table<KeyType, IndexType, LinkType>::start()
{
    return header_.start() && manager_.start();
}

template <typename KeyType, typename IndexType, typename LinkType>
void slab_hash_table<KeyType, IndexType, LinkType>::sync()
{
    return manager_.sync();
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated except in the order written (used by bip30).
template <typename KeyType, typename IndexType, typename LinkType>
LinkType slab_hash_table<KeyType, IndexType, LinkType>::store(
    const KeyType& key, write_function write, size_t value_size)
{
    // Allocate and populate new unlinked slab.
    row slab(manager_);
    const auto position = slab.create(key, write, value_size);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    create_mutex_.lock();

    // Link new slab.next to current first slab.
    slab.link(read_bucket_value(key));

    // Link header to new slab as the new first.
    link(key, position);

    create_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Return the file offset of the slab data segment.
    return position + row::prefix_size;
}

// Execute a writer against a key's buffer if the key is found.
// Return the file offset of the found value (or zero).
template <typename KeyType, typename IndexType, typename LinkType>
LinkType slab_hash_table<KeyType, IndexType, LinkType>::update(
    const KeyType& key, write_function write)
{
    auto slabs = hash_table_iterable<manager, LinkType, row>(manager_,
        read_bucket_value(key), update_mutex_);

    for (slab: slabs)
    {
        // TODO: return row directly from iterator.
        row item(manager_, slab);

        // Found, update data and return position.
        if (item.compare(key))
        {
            const auto memory = item.data();
            auto serial = make_unsafe_serializer(memory->buffer());
            write(serial);
            return item.offset();
        }
    }

    return not_found;
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType, typename IndexType, typename LinkType>
LinkType slab_hash_table<KeyType, IndexType, LinkType>::offset(
    const KeyType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const_row item(manager_, current);

        // Found, return offset.
        if (item.equal(key))
            return item.offset();

        const auto previous = current;
        current = item.next();
        BITCOIN_ASSERT(previous != current);
    }

    return not_found;
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType, typename IndexType, typename LinkType>
memory_ptr slab_hash_table<KeyType, IndexType, LinkType>::find(
    const KeyType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // TODO: implement hash_table_iterable/hash_table_iterator.
    // Iterate through list...
    while (current != not_found)
    {
        const_row item(manager_, current);

        // Found, return data.
        if (item.equal(key))
            return item.data();

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next();
        ///////////////////////////////////////////////////////////////////////
    }

    return nullptr;
}

template <typename KeyType, typename IndexType, typename LinkType>
memory_ptr slab_hash_table<KeyType, IndexType, LinkType>::get(
    LinkType slab) const
{
    return manager_.get(slab);
}

// Unlink is not safe for concurrent write.
// This is limited to unlinking the first of multiple matching key values.
template <typename KeyType, typename IndexType, typename LinkType>
bool slab_hash_table<KeyType, IndexType, LinkType>::unlink(const KeyType& key)
{
    // Find start item...
    auto previous = read_bucket_value(key);

    if (previous == not_found)
        return false;

    row begin_item(manager_, previous);

    // If start item has the key then unlink from buckets.
    if (begin_item.equal(key))
    {
        //*********************************************************************
        const auto next = begin_item.next();
        //*********************************************************************

        link(key, next);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    update_mutex_.lock_shared();
    auto current = begin_item.next();
    update_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // TODO: implement hash_table_iterable/hash_table_iterator.
    // Iterate through list...
    while (current != not_found)
    {
        row item(manager_, current);

        // Found, unlink current item from previous.
        if (item.equal(key))
        {
            row previous_item(manager_, previous);

            // Critical Section
            ///////////////////////////////////////////////////////////////////
            update_mutex_.lock_upgrade();
            const auto next = item.next();
            update_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            previous_item.link(next);
            update_mutex_.unlock();
            ///////////////////////////////////////////////////////////////////
            return true;
        }

        previous = current;

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next();
        ///////////////////////////////////////////////////////////////////////
    }

    return false;
}

// private
template <typename KeyType, typename IndexType, typename LinkType>
IndexType slab_hash_table<KeyType, IndexType, LinkType>::bucket_index(
    const KeyType& key) const
{
    return header::remainder(key, header_.buckets());
}

// private
template <typename KeyType, typename IndexType, typename LinkType>
LinkType slab_hash_table<KeyType, IndexType, LinkType>::read_bucket_value(
    const KeyType& key) const
{
    return header_.read(bucket_index(key));
}

// private
template <typename KeyType, typename IndexType, typename LinkType>
void slab_hash_table<KeyType, IndexType, LinkType>::link(const KeyType& key,
    LinkType begin)
{
    header_.write(bucket_index(key), begin);
}

} // namespace database
} // namespace libbitcoin

#endif
