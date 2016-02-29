/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_HPP

#include <bitcoin/database/memory/memory_array.hpp>
#include <bitcoin/database/hash_table/slab_manager.hpp>

namespace libbitcoin {
namespace database {

/**
 * A hashtable mapping hashes to variable sized values (slabs).
 * Uses a combination of the memory_array and slab_manager.
 *
 * The memory_array is basically a bucket list containing the start
 * value for the hashtable chain.
 *
 * The slab_manager is used to create linked chains. A header
 * containing the hash of the item, and the next value is stored
 * with each slab.
 *
 *   [ HashType ]
 *   [ next:8   ]
 *   [ value... ]
 *
 * If we run allocator.sync() before the link() step then we ensure
 * data can be lost but the hashtable is never corrupted.
 * Instead we prefer speed and batch that operation. The user should
 * call allocator.sync() after a series of store() calls.
 */
template <typename HashType>
class slab_hash_table
{
public:
    typedef std::function<void(uint8_t*)> write_function;

    slab_hash_table(htdb_slab_header& header, slab_manager& allocator);

    /// Store a value. value_size is the requested size for the value.
    /// The provided write() function must write exactly value_size bytes.
    /// Returns the position of the inserted value in the slab_manager.
    file_offset store(const HashType& key, write_function write,
        const size_t value_size);

    /// Return the slab for a given hash.
    uint8_t* get2(const HashType& key) const;

    /// Delete a key-value pair from the hashtable by unlinking the node.
    bool unlink(const HashType& key);

private:

    // What is the bucket given a hash.
    array_index bucket_index(const HashType& key) const;

    // What is the slab start position for a chain.
    file_offset read_bucket_value(const HashType& key) const;

    // Link a new chain into the bucket header.
    void link(const HashType& key, const file_offset begin);

    // Release node from linked chain.
    template <typename ListItem>
    void release(const ListItem& item, const file_offset previous);

    htdb_slab_header& header_;
    slab_manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/slab_hash_table.ipp>

#endif
