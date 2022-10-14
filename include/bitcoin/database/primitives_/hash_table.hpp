/**
/// Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
 *
/// This file is part of libbitcoin.
 *
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
 *
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
 *
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES__HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES__HASH_TABLE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives_/hash_table_header.hpp>
#include <bitcoin/database/primitives_/list.hpp>
#include <bitcoin/database/primitives_/list_element.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// A table mapping hashes to variably-sized values (slabs).
/// Uses a combination of the hash_table and slab_manager.
///
/// The hash_table is a bucket list containing the start value for a list.
///
///  [   size:Index  ]
///  [ [ item:Link ] ]
///  [ [    ...    ] ]
///  [ [ item:Link ] ]
///
/// The slab_manager is used to create a payload of linked chains. A header
/// containing the hash of the item, and the next value is stored with each
/// slab.
///
///   [ key:Key     ]
///   [ next:Link   ]
///   [ record:data ]
///
/// The payload is prefixed with [ size:Link ].
template <class Manager, typename Index, typename Link, typename Key,
    if_unsigned_integer<Index> = true,
    if_unsigned_integer<Link> = true,
    if_integral_array<Key> = true>
class hash_table
{
public:
    typedef list_element<Manager, Link, Key> value_type;
    typedef list_element<const Manager, Link, Key> const_value_type;

    static const Link not_found;

    /// Construct a hash table for variable size entries.
    hash_table(storage& file, Index buckets) NOEXCEPT;

    /// Construct a hash table for fixed size entries.
    hash_table(storage& file, Index buckets, size_t value_size) NOEXCEPT;

    /// Create hash table in the file (left in started state).
    bool create() NOEXCEPT;

    /// Verify the size of the hash table in the file.
    bool start() NOEXCEPT;

    /// Commit table size to the file.
    void commit() NOEXCEPT;

    /// Use to allocate an element in the hash table.
    value_type allocator() NOEXCEPT;

    /// Find an element with the given key in the hash table.
    const_value_type find(const Key& key) const NOEXCEPT;

    /// Get the element with the given link from the hash table.
    const_value_type get(Link link) const NOEXCEPT;

    /// A not found instance for this table, same as find(not_found).
    const_value_type terminator() const NOEXCEPT;

    /// Add the given element to the hash table.
    void link(value_type& element) NOEXCEPT;

    /// Remove an element with the given key from the hash table.
    bool unlink(const Key& key) NOEXCEPT;

private:
    Link bucket_value(Index index) const NOEXCEPT;
    Link bucket_value(const Key& key) const NOEXCEPT;
    Index bucket_index(const Key& key) const NOEXCEPT;

    hash_table_header<Index, Link> header_;
    Manager manager_;
    mutable shared_mutex list_mutex_;
    mutable boost::upgrade_mutex root_mutex_;
};

} // namespace database
} // namespace libbitcoin


#define TEMPLATE \
template <class Manager, typename Index, typename Link, typename Key, \
if_unsigned_integer<Index> If1, if_unsigned_integer<Link> If2, \
if_integral_array<Key> If3>
#define CLASS hash_table<Manager, Index, Link, Key, If1, If2, If3>

#include <bitcoin/database/impl/primitives_/hash_table.ipp>

#undef CLASS
#undef TEMPLATE

#endif
