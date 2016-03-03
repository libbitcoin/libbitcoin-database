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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_HPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_HPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory_map.hpp>

namespace libbitcoin {
namespace database {

/**
 * Implements contigious memory array with a fixed size elements.
 *
 * File format looks like:
 *
 *  [   size:IndexType   ]
 *  [ [      ...       ] ]
 *  [ [ item:ValueType ] ]
 *  [ [      ...       ] ]
 *
 * Empty elements are represented by the value array.empty
 */
template <typename IndexType, typename ValueType>
class hash_table_header
{
public:
    // This VC++ workaround is OK because ValueType must be unsigned. 
    //static constexpr ValueType empty = std::numeric_limits<ValueType>::max();
    static BC_CONSTEXPR ValueType empty = (ValueType)bc::max_uint64;

    hash_table_header(memory_map& file, IndexType buckets);

    /// Allocate the hash table and populate with empty values.
    void create();

    /// Must be called before use. Loads the size from the file.
    void start();

    /// Read item's value.
    ValueType read(IndexType index) const;

    /// Write value to item.
    void write(IndexType index, ValueType value);

    /// The hash table size (bucket count).
    IndexType size() const;

private:

    // Locate the item in the memory map.
    file_offset item_position(IndexType index) const;

    memory_map& file_;
    IndexType buckets_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/hash_table_header.ipp>

#endif
