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
#ifndef LIBBITCOIN_DATABASE_RECORD_MULTIMAP_HPP
#define LIBBITCOIN_DATABASE_RECORD_MULTIMAP_HPP

#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_list.hpp>
#include <bitcoin/database/primitives/record_hash_table.hpp>

namespace libbitcoin {
namespace database {

template <typename HashType>
BC_CONSTEXPR size_t hash_table_multimap_record_size()
{
    return hash_table_record_size<HashType>(sizeof(array_index));
}

/**
 * A multimap hashtable where each key maps to a set of fixed size
 * values.
 *
 * The database is abstracted on top of a record map, and linked records.
 * The map links keys to start indexes in the linked records.
 * The linked records are chains of records that can be iterated through
 * given a start index.
 */
template <typename HashType>
class record_multimap
{
public:
    typedef record_hash_table<HashType> record_hash_table_type;
    typedef std::function<void(memory_ptr)> write_function;

    record_multimap(record_hash_table_type& map, record_list& records);

    /// Lookup a key, returning an iterable result with multiple values.
    array_index lookup(const HashType& key) const;

    /// Add a new row for a key. If the key doesn't exist, it will be created.
    /// If it does exist, the value will be added at the start of the chain.
    void add_row(const HashType& key, write_function write);

    /// Delete the last row entry that was added. This means when deleting
    /// blocks we must walk backwards and delete in reverse order.
    void delete_last_row(const HashType& key);

private:
    // Add new value to existing key.
    void add_to_list(memory_ptr start_info, write_function write);

    // Create new key with a single value.
    void create_new(const HashType& key, write_function write);

    record_hash_table_type& map_;
    record_list& records_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_multimap.ipp>

#endif
