/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_IPP

#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/list.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename Manager, typename Index, typename Link, typename Key>
const Link hash_table<Manager, Index, Link, Key>::not_found = 
    hash_table_header<Index, Link>::empty;

template <typename Manager, typename Index, typename Link, typename Key>
hash_table<Manager, Index, Link, Key>::hash_table(storage& file,
    Index buckets)
  : header_(file, buckets),
    manager_(file, hash_table_header<Index, Link>::size(buckets))
{
}

template <typename Manager, typename Index, typename Link, typename Key>
hash_table<Manager, Index, Link, Key>::hash_table(storage& file,
    Index buckets, size_t value_size)
  : header_(file, buckets),
    manager_(file, hash_table_header<Index, Link>::size(buckets),
        value_type::size(value_size))
{
}

template <typename Manager, typename Index, typename Link, typename Key>
bool hash_table<Manager, Index, Link, Key>::create()
{
    return header_.create() && manager_.create();
}

template <typename Manager, typename Index, typename Link, typename Key>
bool hash_table<Manager, Index, Link, Key>::start()
{
    return header_.start() && manager_.start();
}

template <typename Manager, typename Index, typename Link, typename Key>
void hash_table<Manager, Index, Link, Key>::commit()
{
    return manager_.commit();
}

template <typename Manager, typename Index, typename Link, typename Key>
typename hash_table<Manager, Index, Link, Key>::value_type
hash_table<Manager, Index, Link, Key>::allocator()
{
    return { manager_, list_mutex_ };
}

template <typename Manager, typename Index, typename Link, typename Key>
typename hash_table<Manager, Index, Link, Key>::const_value_type
hash_table<Manager, Index, Link, Key>::find(const Key& key) const
{
    list<const Manager, Link, Key> list(manager_, bucket_value(key),
        list_mutex_);

    for (const auto item: list)
        if (item.match(key))
            return item;

    return *list.end();
}

template <typename Manager, typename Index, typename Link, typename Key>
typename hash_table<Manager, Index, Link, Key>::const_value_type
hash_table<Manager, Index, Link, Key>::find(Link link) const
{
    // Ensure requested position is within the file.
    // We avoid a runtime error here to optimize out the past_eof locks.
    BITCOIN_ASSERT_MSG(!manager_.past_eof(link), "Read past end of file.");

    return { manager_, link, list_mutex_ };
}

template <typename Manager, typename Index, typename Link, typename Key>
typename hash_table<Manager, Index, Link, Key>::const_value_type
hash_table<Manager, Index, Link, Key>::terminator() const
{
    return { manager_, not_found, list_mutex_ };
}

template <typename Manager, typename Index, typename Link, typename Key>
void hash_table<Manager, Index, Link, Key>::link(value_type& element)
{
    const auto index = bucket_index(element.key());

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();
    element.set_next(bucket_value(index));
    root_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    header_.write(index, element.link());
    root_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

// Unlink the first of matching key value.
// Unlink is not executed concurrently with writes.
template <typename Manager, typename Index, typename Link, typename Key>
bool hash_table<Manager, Index, Link, Key>::unlink(const Key& key)
{
    const auto index = bucket_index(key);

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();

    list<Manager, Link, Key> list(manager_, bucket_value(index), list_mutex_);

    if (list.empty())
    {
        root_mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return false;
    }

    auto previous = list.begin();

    // If start item (first in list) has the key then unlink from header.

    // TODO: implement -> overload.
    if ((*previous).match(key))
    {
        root_mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        header_.write(index, (*previous).next());
        root_mutex_.unlock();
        //---------------------------------------------------------------------
        return true;
    }

    root_mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    // The linked list internally manages link update safety using list_mutex_.
    for (auto item = ++previous; item != list.end(); item++)
    {
        // TODO: implement -> overloads.
        if ((*item).match(key))
        {
            (*previous).set_next((*item).next());
            return true;
        }
    }

    return false;
}

// private
template <typename Manager, typename Index, typename Link, typename Key>
Link hash_table<Manager, Index, Link, Key>::bucket_value(Index index) const
{
    return header_.read(index);
}

// private
template <typename Manager, typename Index, typename Link, typename Key>
Link hash_table<Manager, Index, Link, Key>::bucket_value(const Key& key) const
{
    return header_.read(bucket_index(key));
}

// private
template <typename Manager, typename Index, typename Link, typename Key>
Index hash_table<Manager, Index, Link, Key>::bucket_index(const Key& key) const
{
    return hash_table_header<Index, Link>::remainder(key, header_.buckets());
}

} // namespace database
} // namespace libbitcoin

#endif
