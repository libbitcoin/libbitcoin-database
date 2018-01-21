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
#ifndef LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/linked_list.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/**
 * A hash table mapping hashes to variable sized values (slabs).
 * Uses a combination of the hash_table and slab_manager.
 *
 * The hash_table is basically a bucket list containing the start value for the
 * linked_list.
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
template <typename Key, typename Index, typename Link>
class slab_hash_table
  : noncopyable
{
public:
    typedef byte_serializer::functor write_function;

    static const Link not_found = hash_table_header<Index, Link>::empty;

    /// Construct a hash table for variable size entries.
    slab_hash_table(storage& file, Index buckets);

    /// Create hash table in the file (left in started state).
    bool create();

    /// Verify the size of the hash table in the file.
    bool start();

    /// Commit changes to the hash table.
    void sync();

    /// Execute a write. value_size is the required size of the buffer.
    /// Returns the file offset of the new value.
    Link store(const Key& key, write_function write, size_t size);

    /// Execute a writer against a key's buffer if the key is found.
    /// Returns the file offset of the found value (or not_found).
    Link update(const Key& key, write_function write);

    /// Find the file offset for a given key. Returns not_found if not found.
    Link offset(const Key& key) const;

    /// Find the slab pointer for a given key. Returns nullptr if not found.
    memory_ptr find(const Key& key) const;

    /// Get slab from its file offset. Returns nullptr if not found.
    memory_ptr get(Link slab) const;

    /// Delete a key-value pair from the hashtable by unlinking the node.
    bool unlink(const Key& key);

private:
    typedef hash_table_header<Index, Link> header;
    typedef slab_manager<Link> manager;
    typedef linked_list<manager, Link, Key> row;
    typedef linked_list<const manager, Link, Key> const_row;

    // The bucket index of a key.
    Index bucket_index(const Key& key) const;

    // The slab start position for the set of elements mapped to the key.
    Link read_bucket_value(const Key& key) const;

    // Link a new element into the bucket header (stack model, push front).
    void link(const Key& key, Link begin);

    header header_;
    manager manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/slab_hash_table.ipp>

#endif
