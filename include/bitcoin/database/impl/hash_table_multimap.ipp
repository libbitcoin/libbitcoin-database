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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_MULTIMAP_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_MULTIMAP_IPP

#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/list.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>

namespace libbitcoin {
namespace database {

// static
template <typename Key, typename Index, typename Link>
size_t hash_table_multimap<Key, Index, Link>::size(size_t value_size)
{
    return value_type::size(value_size);
}

template <typename Key, typename Index, typename Link>
hash_table_multimap<Key, Index, Link>::hash_table_multimap(table& map,
    manager& manager)
  : map_(map), manager_(manager)
{
}

template <typename Key, typename Index, typename Link>
typename hash_table_multimap<Key, Index, Link>::value_type
hash_table_multimap<Key, Index, Link>::allocator()
{
    // Empty-keyed (for payload elements).
    return { manager_, list_mutex_ };
}

template <typename Key, typename Index, typename Link>
typename hash_table_multimap<Key, Index, Link>::list
hash_table_multimap<Key, Index, Link>::find(const Key& key) const
{
    const auto element = map_.find(key);

    if (!element)
        return { manager_, element.not_found, list_mutex_ };

    Link first;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(root_mutex_);
        first = deserial.template read_little_endian<Link>();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    return { manager_, first, list_mutex_ };
}

template <typename Key, typename Index, typename Link>
typename hash_table_multimap<Key, Index, Link>::list
hash_table_multimap<Key, Index, Link>::find(Link link) const
{
    const auto element = map_.find(link);

    if (!element)
        return { manager_, element.not_found, list_mutex_ };

    Link first;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(root_mutex_);
        first = deserial.template read_little_endian<Link>();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    return { manager_, first, list_mutex_ };
}

template <typename Key, typename Index, typename Link>
void hash_table_multimap<Key, Index, Link>::link(const Key& key,
    value_type& element)
{
    const auto writer = [&](byte_serializer& serial)
    {
        serial.template write_little_endian<Link>(element.link());
    };

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();

    // Find the root element for this key in hash table.
    // The hash table supports multiple values per key, but this uses only one.
    const auto roots = find(key);

    root_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (roots.empty())
    {
        // Commit the termination of the new list.
        element.next(element.not_found);

        // Create and map new root and "link" from it to the new element.
        auto root = map_.allocator();
        root.create(key, writer);
        map_.link(root);
    }
    else
    {
        Link first;
        auto root = roots.front();
        const auto reader = [&](byte_deserializer& deserial)
        {
            first = deserial.template read_little_endian<Link>();
        };

        // Read the address of the existing first list element.
        root.read(reader);

        // Commit linkage to the existing first list element.
        element.next(first);

        // "link" existing root to the new first element.
        root.write(writer);
    }

    root_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Key, typename Index, typename Link>
bool hash_table_multimap<Key, Index, Link>::unlink(const Key& key)
{
    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();

    // Find the root element for this key in hash table.
    // The hash table supports multiple values per key, but this uses only one.
    const auto roots = find(key);

    if (roots.empty())
    {
        root_mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return false;
    }

    // Define link writer for parent element.
    const auto linker = [&](byte_serializer& serial)
    {
        serial.template write_little_endian<Link>(roots.front().next());
    };

    root_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    roots.front().write(linker);

    root_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
