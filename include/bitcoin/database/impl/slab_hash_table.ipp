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

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/linked_list_iterable.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename Manager, typename Key, typename Index, typename Link>
slab_hash_table<Manager, Key, Index, Link>::slab_hash_table(storage& file,
    Index buckets)
  : header_(file, buckets),
    manager_(file, hash_table_header<Index, Link>::size(buckets))
{
}

template <typename Manager, typename Key, typename Index, typename Link>
slab_hash_table<Manager, Key, Index, Link>::slab_hash_table(storage& file,
    Index buckets, size_t value_size)
  : header_(file, buckets),
    manager_(file, hash_table_header<Index, Link>::size(buckets), value_size)
{
}

template <typename Manager, typename Key, typename Index, typename Link>
bool slab_hash_table<Manager, Key, Index, Link>::create()
{
    return header_.create() && manager_.create();
}

template <typename Manager, typename Key, typename Index, typename Link>
bool slab_hash_table<Manager, Key, Index, Link>::start()
{
    return header_.start() && manager_.start();
}

template <typename Manager, typename Key, typename Index, typename Link>
void slab_hash_table<Manager, Key, Index, Link>::commit()
{
    return manager_.commit();
}

template <typename Manager, typename Key, typename Index, typename Link>
typename slab_hash_table<Manager, Key, Index, Link>::value_type
slab_hash_table<Manager, Key, Index, Link>::allocator()
{
    return { manager_, list_mutex_ };
}

template <typename Manager, typename Key, typename Index, typename Link>
typename slab_hash_table<Manager, Key, Index, Link>::const_value_type
slab_hash_table<Manager, Key, Index, Link>::find(const Key& key) const
{
    // manager_ is const.
    linked_list_iterable<const Manager, Link, Key> list(manager_,
        bucket_value(key), list_mutex_);

    for (const auto item: list)
        if (item.match(key))
            return item;

    return *list.end();
}

template <typename Manager, typename Key, typename Index, typename Link>
typename slab_hash_table<Manager, Key, Index, Link>::const_value_type
slab_hash_table<Manager, Key, Index, Link>::find(Link link) const
{
    return { manager_, link, list_mutex_ };
}

template <typename Manager, typename Key, typename Index, typename Link>
void slab_hash_table<Manager, Key, Index, Link>::link(value_type& element)
{
    const auto index = bucket_index(element.key());

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();
    element.next(bucket_value(index));
    root_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    header_.write(index, element.link());
    root_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

// Unlink the first of matching key value.
// Unlink is not executed concurrently with writes.
template <typename Manager, typename Key, typename Index, typename Link>
bool slab_hash_table<Manager, Key, Index, Link>::unlink(const Key& key)
{
    const auto index = bucket_index(key);

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();

    // manager_ is const.
    linked_list_iterable<Manager, Link, Key> list(manager_,
        bucket_value(index), list_mutex_);

    if (list.empty())
    {
        root_mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return false;
    }

    auto previous = list.begin();

    // If start item (first in list) has the key then unlink from header.

    // TODO: implement -> overload.
    auto foo = *previous;

    if (foo.match(key))
    {
        root_mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        header_.write(index, foo.next());
        root_mutex_.unlock();
        //---------------------------------------------------------------------
        return true;
    }

    root_mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    // The linked list internally manages link update safety using list_mutex_.
    for (auto item = ++previous;

        // TODO: implement != overload.
        item != list.end();

        item++)
    {
        // TODO: implement -> overload.
        auto baz = *item;

        if (baz.match(key))
        {
            // TODO: implement -> overload.
            auto bar = *previous;

            bar.next(baz.next());
            return true;
        }
    }

    return false;
}

// private
template <typename Manager, typename Key, typename Index, typename Link>
Link slab_hash_table<Manager, Key, Index, Link>::bucket_value(
    Index index) const
{
    return header_.read(index);
}

// private
template <typename Manager, typename Key, typename Index, typename Link>
Link slab_hash_table<Manager, Key, Index, Link>::bucket_value(
    const Key& key) const
{
    return header_.read(bucket_index(key));
}

// private
template <typename Manager, typename Key, typename Index, typename Link>
Index slab_hash_table<Manager, Key, Index, Link>::bucket_index(
    const Key& key) const
{
    return hash_table_header<Index, Link>::remainder(key, header_.buckets());
}

} // namespace database
} // namespace libbitcoin

#endif
