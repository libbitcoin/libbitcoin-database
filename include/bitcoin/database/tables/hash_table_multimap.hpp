/////**
//// * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_MULTIMAP_HPP
////#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_MULTIMAP_HPP
////
////#include <bitcoin/system.hpp>
////#include <bitcoin/database/boost.hpp>
////#include <bitcoin/database/define.hpp>
////#include <bitcoin/database/memory/memory.hpp>
////#include <bitcoin/database/tables/hash_table.hpp>
////#include <bitcoin/database/tables/list_element.hpp>
////#include <bitcoin/database/tables/record_manager.hpp>
////
////namespace libbitcoin {
////namespace database {
////
/////// A hash table where each key maps to a set of fixed size values.
///////
/////// The database is abstracted on top of a record map, and linked records.
/////// The map links keys to start indexes in the linked records.
/////// The linked records are chains of records that can be iterated through
/////// given a start index.
////template <typename Index, typename Link, typename Key,
////    if_unsigned_integer<Index> = true,
////    if_unsigned_integer<Link> = true,
////    if_integral_array<Key> = true>
////class hash_table_multimap
////{
////public:
////    typedef record_manager<Link> manager;
////    typedef list_element<manager, Link, empty_key> value_type;
////    typedef list_element<const manager, Link, empty_key> const_value_type;
////    typedef hash_table<manager, Index, Link, Key> table;
////
////    /// The stored size of a record value with the given size.
////    static size_t size(size_t value_size) NOEXCEPT;
////
////    /// Construct a new record multimap.
////    /// THIS ASSUMES MAP HAS VALUE SIZE == sizeof(Link).
////    hash_table_multimap(table& map, manager& manager) NOEXCEPT;
////
////    /// Use to allocate an element in a multimap.
////    value_type allocator() NOEXCEPT;
////
////    /// Find an iterator for the given multimap key.
////    const_value_type find(const Key& key) const NOEXCEPT;
////
////    /// Get the iterator for the given link from a multimap.
////    const_value_type get(Link link) const NOEXCEPT;
////
////    /// Add the given element to a multimap.
////    /// Multimap elements have empty internal key values.
////    void link(const Key& key, value_type& element) NOEXCEPT;
////
////    /// Remove a multimap element with the given key.
////    bool unlink(const Key& key) NOEXCEPT;
////
////private:
////    table& map_;
////    manager& manager_;
////    mutable upgrade_mutex root_mutex_;
////    mutable shared_mutex list_mutex_;
////};
////
////} // namespace database
////} // namespace libbitcoin
////
////
////#define TEMPLATE \
////template <typename Index, typename Link, typename Key, \
////if_unsigned_integer<Index> If1, if_unsigned_integer<Link> If2, \
////if_integral_array<Key> If3>
////#define CLASS hash_table_multimap<Index, Link, Key, If1, If2, If3>
////
////#include <bitcoin/database/impl/tables/hash_table_multimap.ipp>
////
////#undef CLASS
////#undef TEMPLATE
////
////#endif
