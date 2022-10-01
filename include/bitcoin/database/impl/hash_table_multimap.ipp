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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_MULTIMAP_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_MULTIMAP_IPP

#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/list.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

// static
template <typename Index, typename Link, typename Key>
size_t hash_table_multimap<Index, Link, Key>::size(size_t value_size)
{
    return value_type::size(value_size);
}

template <typename Index, typename Link, typename Key>
hash_table_multimap<Index, Link, Key>::hash_table_multimap(table& map,
    manager& manager)
  : map_(map), manager_(manager)
{
}

template <typename Index, typename Link, typename Key>
typename hash_table_multimap<Index, Link, Key>::value_type
hash_table_multimap<Index, Link, Key>::allocator()
{
    // Empty-keyed (for payload elements).
    return { manager_, list_mutex_ };
}

template <typename Index, typename Link, typename Key>
typename hash_table_multimap<Index, Link, Key>::const_value_type
hash_table_multimap<Index, Link, Key>::find(const Key& key) const
{
    const auto element = map_.find(key);

    if (!element)
        return { manager_, element.not_found, list_mutex_ };

    Link first;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        std::shared_lock lock(root_mutex_);
        first = deserial.template read_little_endian<Link>();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    return { manager_, first, list_mutex_ };
}

template <typename Index, typename Link, typename Key>
typename hash_table_multimap<Index, Link, Key>::const_value_type
hash_table_multimap<Index, Link, Key>::get(Link link) const
{
    const auto element = map_.get(link);

    if (!element)
        return { manager_, element.not_found, list_mutex_ };

    Link first;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        std::shared_lock lock(root_mutex_);
        first = deserial.template read_little_endian<Link>();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    return { manager_, first, list_mutex_ };
}

template <typename Index, typename Link, typename Key>
void hash_table_multimap<Index, Link, Key>::link(const Key& key,
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
    auto root = map_.find(key);

    root_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (!root)
    {
        // Commit the termination of the new list.
        element.set_next(element.not_found);

        // Create and map new root and "link" from it to the new element.
        auto new_root = map_.allocator();
        new_root.create(key, writer);
        map_.link(new_root);
    }
    else
    {
        Link first;
        const auto reader = [&](byte_deserializer& deserial)
        {
            // This could be a terminator if previously unlinked.
            first = deserial.template read_little_endian<Link>();
        };

        // Read the address of the existing first list element.
        root.read(reader);

        // Commit linkage to the existing first list element.
        element.set_next(first);

        // "link" existing root to the new first element.
        root.write(writer);
    }

    root_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Index, typename Link, typename Key>
bool hash_table_multimap<Index, Link, Key>::unlink(const Key& key)
{
    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    root_mutex_.lock_upgrade();

    // Find the root element for this key in hash table.
    // The hash table supports multiple values per key, but this uses only one.
    auto root = map_.find(key);

    // There is no root element, nothing to unlink.
    if (!root)
    {
        root_mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return false;
    }

    Link link;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // This could be a terminator if previously unlinked.
        link = deserial.template read_little_endian<Link>();
    };

    // Read the address of the existing first list element.
    root.read(reader);

    value_type first{ manager_, link, list_mutex_ };

    // The root element is empty (points to terminator), nothing to unlink.
    if (!first)
    {
        root_mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return false;
    }

    // This may leave an empty root element in place, but presumably that will
    // become resused in the future as the transaction is re-confirmed.
    const auto writer = [&](byte_serializer& serial)
    {
        // Skip over the first element pointed to by the root.
        serial.template write_little_endian<Link>(first.next());
    };

    root_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    root.write(writer);

    root_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
