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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_MULTIMAP_HPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_MULTIMAP_HPP

#include <cstdint>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/list.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

/**
 * A hash table where each key maps to a set of fixed size values.
 *
 * The database is abstracted on top of a record map, and linked records.
 * The map links keys to start indexes in the linked records.
 * The linked records are chains of records that can be iterated through
 * given a start index.
 */
template <typename Key, typename Index, typename Link>
class hash_table_multimap
{
public:
    typedef record_manager<Link> manager;
    typedef list_element<manager, Link, empty_key> value_type;
    typedef list_element<const manager, Link, empty_key> const_value_type;
    typedef hash_table<manager, Key, Index, Link> table;

    /// The stored size of a record value with the given size.
    static size_t size(size_t value_size);

    /// Construct a new record multimap.
    /// THIS ASSUMES MAP HAS VALUE == sizeof(Link).
    hash_table_multimap(table& map, manager& manager);

    /// Use to allocate an element in a multimap. 
    value_type allocator();

    /// Find an iterator for the given multimap key.
    const_value_type find(const Key& key) const;

    /// Get the iterator for the given link from a multimap.
    const_value_type find(Link link) const;

    /// Add the given element to a multimap.
    /// Multimap elements have empty internal key values.
    void link(const Key& key, value_type& element);

    /// Remove a multimap element with the given key.
    bool unlink(const Key& key);

private:
    table& map_;
    manager& manager_;
    mutable shared_mutex root_mutex_;
    mutable shared_mutex list_mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/hash_table_multimap.ipp>

#endif
