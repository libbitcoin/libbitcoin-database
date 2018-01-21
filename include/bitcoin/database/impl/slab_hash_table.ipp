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
#include <bitcoin/database/primitives/linked_list.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename Key, typename Index, typename Link>
slab_hash_table<Key, Index, Link>::slab_hash_table(storage& file,
    Index buckets)
  : header_(file, buckets),
    manager_(file, header::size(buckets))
{
}

template <typename Key, typename Index, typename Link>
bool slab_hash_table<Key, Index, Link>::create()
{
    return header_.create() && manager_.create();
}

template <typename Key, typename Index, typename Link>
bool slab_hash_table<Key, Index, Link>::start()
{
    return header_.start() && manager_.start();
}

template <typename Key, typename Index, typename Link>
void slab_hash_table<Key, Index, Link>::sync()
{
    return manager_.sync();
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated except in the order written (used by bip30).
template <typename Key, typename Index, typename Link>
Link slab_hash_table<Key, Index, Link>::store(const Key& key,
    write_function write, size_t value_size)
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

    // TODO: change to the position of the record (starting at key, not value).

    // Return the file offset of the slab data segment.
    return position + row::prefix_size;
}

// Execute a writer against a key's buffer if the key is found.
// Return the file offset of the found value (or zero).
template <typename Key, typename Index, typename Link>
Link slab_hash_table<Key, Index, Link>::update(const Key& key,
    write_function write)
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != not_found)
    {
        row item(manager_, current);

        // Found, update data and return position.
        if (item.equal(key))
        {
            const auto memory = item.data();
            auto serial = make_unsafe_serializer(memory->buffer());
            write(serial);
            return item.offset();
        }

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next();
        ///////////////////////////////////////////////////////////////////////
    }

    return not_found;
}

// This is limited to returning the first of multiple matching key values.
template <typename Key, typename Index, typename Link>
Link slab_hash_table<Key, Index, Link>::offset(const Key& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != not_found)
    {
        const_row item(manager_, current);

        // Found, return offset.
        if (item.equal(key))
            return item.offset();

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next();
        ///////////////////////////////////////////////////////////////////////
    }

    return not_found;
}

// This is limited to returning the first of multiple matching key values.
template <typename Key, typename Index, typename Link>
memory_ptr slab_hash_table<Key, Index, Link>::find(const Key& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

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

template <typename Key, typename Index, typename Link>
memory_ptr slab_hash_table<Key, Index, Link>::get(Link slab) const
{
    return manager_.get(slab);
}

// Unlink is not safe for concurrent write.
// This is limited to unlinking the first of multiple matching key values.
template <typename Key, typename Index, typename Link>
bool slab_hash_table<Key, Index, Link>::unlink(const Key& key)
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
template <typename Key, typename Index, typename Link>
Index slab_hash_table<Key, Index, Link>::bucket_index(const Key& key) const
{
    return header::remainder(key, header_.buckets());
}

// private
template <typename Key, typename Index, typename Link>
Link slab_hash_table<Key, Index, Link>::read_bucket_value(const Key& key) const
{
    return header_.read(bucket_index(key));
}

// private
template <typename Key, typename Index, typename Link>
void slab_hash_table<Key, Index, Link>::link(const Key& key, Link begin)
{
    header_.write(bucket_index(key), begin);
}

} // namespace database
} // namespace libbitcoin

#endif
