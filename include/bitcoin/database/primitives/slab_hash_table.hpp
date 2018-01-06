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
#include <bitcoin/database/primitives/slab_manager.hpp>

namespace libbitcoin {
namespace database {

/**
 * A hashtable mapping hashes to variable sized values (slabs).
 * Uses a combination of the hash_table and slab_manager.
 *
 * The hash_table is basically a bucket list containing the start
 * value for the slab_row.
 *
 * The slab_manager is used to create linked chains. A header
 * containing the hash of the item, and the next value is stored
 * with each slab.
 *
 *   [ KeyType  ]
 *   [ next:8   ]
 *   [ value... ]
 *
 * If we run manager.sync() before the link() step then we ensure
 * data can be lost but the hashtable is never corrupted.
 * Instead we prefer speed and batch that operation. The user should
 * call allocator.sync() after a series of store() calls.
 */
template <typename KeyType>
class slab_hash_table
{
public:
    typedef KeyType key_type;
    typedef byte_serializer::functor write_function;

    // This determines the nature of the hash table.
    // These template parameters could be moved up to slab_hash_table.
    typedef hash_table_header<array_index, file_offset> header_type;

    typedef header_type::value_type link_type;
    typedef header_type::index_type index_type;
    typedef slab_manager<link_type> slab_manager;
    static const link_type not_found = header_type::empty;

    /// Construct a hash table for variable size entries.
    slab_hash_table(header_type& header, slab_manager& manager);

    /// Execute a write. value_size is the required size of the buffer.
    /// Returns the file offset of the new value.
    link_type store(const KeyType& key, write_function write, size_t size);

    /// Execute a writer against a key's buffer if the key is found.
    /// Returns the file offset of the found value (or not_found).
    link_type update(const KeyType& key, write_function write);

    /// Find the file offset for a given key. Returns not_found if not found.
    link_type offset(const KeyType& key) const;

    /// Find the slab pointer for a given key. Returns nullptr if not found.
    memory_ptr find(const KeyType& key) const;

    /// Delete a key-value pair from the hashtable by unlinking the node.
    bool unlink(const KeyType& key);

private:
    // The bucket index of a key.
    index_type bucket_index(const KeyType& key) const;

    // The slab start position for the set of elements mapped to the key.
    link_type read_bucket_value(const KeyType& key) const;

    // Link a new element into the bucket header (stack model, push front).
    void link(const KeyType& key, link_type begin);

    header_type& header_;
    slab_manager& manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/slab_hash_table.ipp>

#endif
