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
#ifndef LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/record_row.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/**
 * A hash table mapping hashes to fixed sized values (records).
 * Uses a combination of the hash_table and record_manager.
 *
 * By using the record_manager instead of slabs, we can have smaller
 * indexes avoiding reading/writing extra bytes to the file.
 * Using fixed size records is therefore faster.
 *
 * The hash_table is basically a bucket list containing the start value for the
 * record_row.
 *
 *  [   size:IndexType  ]
 *  [ [ item:LinkType ] ]
 *  [ [      ...      ] ]
 *  [ [ item:LinkType ] ]
 *
 * The record_manager is used to create a payload of linked chains. A header
 * containing the hash of the item, and the next value is stored with each
 * record.
 *
 *   [ key:KeyType   ]
 *   [ next:LinkType ]
 *   [ record:data   ]
 *
 * The payload is prefixed with [ count:LinkType ].
 */
template <typename KeyType, typename IndexType, typename LinkType>
class record_hash_table
  : noncopyable
{
public:
    typedef byte_serializer::functor write_function;

    static const LinkType not_found = hash_table_header<IndexType,
        LinkType>::empty;

    /// Construct a hash table for uniform size entries.
    record_hash_table(storage& file, IndexType buckets, size_t value_size);

    /// Create hash table in the file (left in started state).
    bool create();

    /// Verify the size of the hash table in the file.
    bool start();

    /// Commit changes to the hash table.
    void sync();

    /// Execute a write. The provided write() function must write the correct
    /// size: (value_size = record_size - key_size - sizeof(array_index)).
    LinkType store(const KeyType& key, write_function write);

    /// Execute a writer against a key's buffer if the key is found.
    /// Returns the array offset of the found value (or not_found).
    LinkType update(const KeyType& key, write_function write);

    /// Find the array offset for given key. Returns not_found if not found.
    LinkType offset(const KeyType& key) const;

    /// Find the record for given key. Returns nullptr if not found.
    memory_ptr find(const KeyType& key) const;

    /// Get record from its array offset. Returns nullptr if not found.
    memory_ptr get(LinkType record) const;

    /// Delete a key-value pair from the hashtable by unlinking the node.
    bool unlink(const KeyType& key);

private:
    typedef hash_table_header<IndexType, LinkType> header;
    typedef record_manager<LinkType> manager;
    typedef record_row<KeyType, LinkType, manager> row;
    typedef record_row<KeyType, LinkType, const manager> const_row;

    // The bucket index of a key.
    IndexType bucket_index(const KeyType& key) const;

    // The record start position for the set of elements mapped to the key.
    LinkType read_bucket_value(const KeyType& key) const;

    // Link a new element into the bucket header (stack model, push front).
    void link(const KeyType& key, LinkType begin);

    header header_;
    manager manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_hash_table.ipp>

#endif
