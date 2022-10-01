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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HPP

#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/**
 * A hash table mapping hashes to variable sized values (slabs).
 * Uses a combination of the hash_table and slab_manager.
 *
 * The hash_table is basically a bucket list containing the start value for the
 * list_element.
 *
 *  [   size:Index  ]
 *  [ [ item:Link ] ]
 *  [ [    ...    ] ]
 *  [ [ item:Link ] ]
 *
 * The slab_manager is used to create a payload of linked chains. A header
 * containing the hash of the item, and the next value is stored with each
 * slab.
 *
 *   [ key:Key     ]
 *   [ next:Link   ]
 *   [ record:data ]
 *
 * The payload is prefixed with [ size:Link ].
 */
template <typename Manager, typename Index, typename Link, typename Key>
class hash_table
{
public:
    typedef list_element<Manager, Link, Key> value_type;
    typedef list_element<const Manager, Link, Key> const_value_type;

    /// Construct a hash table for variable size entries.
    static const Link not_found;

    /// Construct a hash table for variable size entries.
    hash_table(storage& file, Index buckets);

    /// Construct a hash table for fixed size entries.
    hash_table(storage& file, Index buckets, size_t value_size);

    /// Create hash table in the file (left in started state).
    bool create();

    /// Verify the size of the hash table in the file.
    bool start();

    /// Commit table size to the file.
    void commit();

    /// Use to allocate an element in the hash table.
    value_type allocator();

    /// Find an element with the given key in the hash table.
    const_value_type find(const Key& key) const;

    /// Get the element with the given link from the hash table.
    const_value_type get(Link link) const;

    /// A not found instance for this table, same as find(not_found).
    const_value_type terminator() const;

    /// Add the given element to the hash table.
    void link(value_type& element);

    /// Remove an element with the given key from the hash table.
    bool unlink(const Key& key);

private:
    Link bucket_value(Index index) const;
    Link bucket_value(const Key& key) const;
    Index bucket_index(const Key& key) const;

    hash_table_header<Index, Link> header_;
    Manager manager_;
    mutable std::shared_mutex root_mutex_;
    mutable std::shared_mutex list_mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/hash_table.ipp>

#endif
