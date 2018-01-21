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
#ifndef LIBBITCOIN_DATABASE_RECORD_MULTIMAP_HPP
#define LIBBITCOIN_DATABASE_RECORD_MULTIMAP_HPP

#include <cstdint>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/iterable.hpp>
#include <bitcoin/database/primitives/record_hash_table.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

/**
 * A multimap hashtable where each key maps to a set of fixed size values.
 *
 * The database is abstracted on top of a record map, and linked records.
 * The map links keys to start indexes in the linked records.
 * The linked records are chains of records that can be iterated through
 * given a start index.
 */
template <typename KeyType, typename IndexType, typename Link>
class record_multimap
  : noncopyable
{
public:
    typedef serializer<uint8_t*>::functor write_function;

    /// The stored size of a value with the given size.
    static size_t size(size_t value_size);

    /// Construct a new mutimap.
    record_multimap(record_hash_table<KeyType, IndexType, Link>& map,
        record_manager<Link>& manager);

    /// Add a new element for a key.
    void store(const KeyType& key, write_function write);

    /// Get an iterator for the key.
    iterable<record_manager<Link>, Link> find(const KeyType& key) const;

    /// Get a remap safe address pointer to key's data.
    memory_ptr get(Link index) const;

    /// Delete the last element that was added for the key.
    bool unlink(const KeyType& key);

private:
    typedef linked_list<record_manager<Link>, Link> row_manager;
    record_hash_table<KeyType, IndexType, Link>& map_;
    record_manager<Link>& manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_multimap.ipp>

#endif
