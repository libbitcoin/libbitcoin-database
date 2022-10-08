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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASH_TABLE_HEADER_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASH_TABLE_HEADER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// Size-prefixed array.
/// Empty elements are represented by the value hash_table_header.empty.
///
///  [  size:Index  ]
///  [ [ row:Link ] ]
///  [ [    ...   ] ]
///  [ [ row:Link ] ]
///
template <typename Index, typename Link,
    if_unsigned_integer<Index> = true,
    if_unsigned_integer<Link> = true>
class hash_table_header
{
public:
    /// Empty cell (null pointer) sentinel.
    static constexpr Link empty = system::maximum<Link>;

    /// A hash of the key reduced to the domain of the divisor.
    template <typename Key>
    static constexpr Index remainder(const Key& key, Index divisor) NOEXCEPT
    {
        return divisor == 0 ? 0 : system::djb2_hash(key) % divisor;
    }

    /// The hash table header byte size for a given bucket count.
    static size_t size(Index buckets) NOEXCEPT;

    /// Construct a hash table header.
    hash_table_header(storage& file, Index buckets) NOEXCEPT;

    /// Allocate the hash table and populate with empty values.
    bool create() NOEXCEPT;

    /// Should be called before use. Validates the size from the file.
    bool start() NOEXCEPT;

    /// Read item value.
    Link read(Index index) const NOEXCEPT;

    /// Write value to item.
    void write(Index index, Link value) NOEXCEPT;

    /// The hash table header bucket count.
    Index buckets() const NOEXCEPT;

    /// The hash table header byte size.
    size_t size() NOEXCEPT;

private:
    // Position in the memory map relative the header end.
    static file_offset link(Index index) NOEXCEPT;

    storage& file_;
    Index buckets_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin


#define TEMPLATE \
template <typename Index, typename Link, \
if_unsigned_integer<Index> If1, if_unsigned_integer<Link> If2>
#define CLASS hash_table_header<Index, Link, If1, If2>

#include <bitcoin/database/impl/primitives/hash_table_header.ipp>

#undef CLASS
#undef TEMPLATE

#endif
