/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES__HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES__HASH_TABLE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
const Link CLASS::not_found = hash_table_header<Index, Link>::empty;

TEMPLATE
CLASS::hash_table(storage& file, Index buckets) NOEXCEPT
  : header_(file, buckets),
    manager_(file, hash_table_header<Index, Link>::size(buckets))
{
}

TEMPLATE
CLASS::hash_table(storage& file, Index buckets, size_t value_size) NOEXCEPT
  : header_(file, buckets),
    manager_(file, hash_table_header<Index, Link>::size(buckets),
        value_type::size(value_size))
{
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    return header_.create() && manager_.create();
}

TEMPLATE
bool CLASS::start() NOEXCEPT
{
    return header_.start() && manager_.start();
}

TEMPLATE
void CLASS::commit() NOEXCEPT
{
    return manager_.commit();
}

TEMPLATE
typename CLASS::value_type
CLASS::allocator() NOEXCEPT
{
    return { manager_, list_mutex_ };
}

TEMPLATE
typename CLASS::const_value_type
CLASS::find(const Key& key) const NOEXCEPT
{
    list<const Manager, Link, Key> list(manager_, bucket_value(key),
        list_mutex_);

    for (const auto item: list)
        if (item.match(key))
            return item;

    return *list.end();
}

TEMPLATE
typename CLASS::const_value_type
CLASS::get(Link link) const NOEXCEPT
{
    // Ensure requested position is within the file.
    // We avoid a runtime error here to optimize out the past_eof locks.
    BC_ASSERT_MSG(!manager_.past_eof(link) || link == not_found,
        "Non-terminating link is past end of file.");

    // A not_found link value produces a terminator element.
    return { manager_, link, list_mutex_ };
}

TEMPLATE
typename CLASS::const_value_type
CLASS::terminator() const NOEXCEPT
{
    return { manager_, not_found, list_mutex_ };
}

TEMPLATE
void CLASS::link(value_type& element) NOEXCEPT
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
TEMPLATE
bool CLASS::unlink(const Key& key) NOEXCEPT
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
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::bucket_value(Index index) const NOEXCEPT
{
    return header_.read(index);
}

TEMPLATE
Link CLASS::bucket_value(const Key& key) const NOEXCEPT
{
    return header_.read(bucket_index(key));
}

TEMPLATE
Index CLASS::bucket_index(const Key& key) const NOEXCEPT
{
    return hash_table_header<Index, Link>::remainder(key, header_.buckets());
}

} // namespace database
} // namespace libbitcoin

#endif
