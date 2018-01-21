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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_HPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_HPP

#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// Size-prefixed array.
/// Empty elements are represented by the value hash_table_header.empty.
///
///  [  size:Index  ]
///  [ [ row:Link ] ]
///  [ [      ...     ] ]
///  [ [ row:Link ] ]
///
template <typename Index, typename Link>
class hash_table_header
  : noncopyable
{
public:
    /// A hash of the key reduced to the domain of the divisor.
    template <typename Key>
    static Index remainder(const Key& key, Index divisor);

    // This cast is a VC++ workaround is OK because Link must be unsigned.
    //static constexpr Link empty = std::numeric_limits<Link>::max();
    static const Link empty = (Link)bc::max_uint64;

    /// The hash table header byte size for a given bucket count.
    static size_t size(Index buckets);

    /// Construct a hash table header.
    hash_table_header(storage& file, Index buckets);

    /// Allocate the hash table and populate with empty values.
    bool create();

    /// Should be called before use. Validates the size from the file.
    bool start();

    /// Read item value.
    Link read(Index index) const;

    /// Write value to item.
    void write(Index index, Link value);

    /// The hash table header bucket count.
    Index buckets() const;

    /// The hash table header byte size.
    size_t size();

private:
    // Position in the memory map relative the header end.
    static file_offset offset(Index index);

    storage& file_;
    Index buckets_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/hash_table_header.ipp>

#endif
