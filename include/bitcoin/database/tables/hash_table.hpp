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
#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/hash_table_header.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename Element>
class hash_table
{
public:
    using link = typename Element::link;
    using key = typename Element::key;

    hash_table(storage& header, storage& body, link buckets) NOEXCEPT;

    /// Not thread safe.
    bool create() NOEXCEPT { return false; }

    /// Thread safe.
    Element at(link link) const NOEXCEPT;
    Element find(const key& key) const NOEXCEPT;

    // push size records or size slab bytes.
    // size is contiguous body allocation, only first is linked.
    Element push(const key& key, link size=one) NOEXCEPT;

private:
    using header = hash_table_header<link, key>;
    using manager = typename Element::manager;

    // hash/head/push thread safe.
    header header_;

    // Thread safe.
    manager body_;
};

} // namespace database
} // namespace libbitcoin


#define TEMPLATE \
template <typename Element>
#define CLASS hash_table<Element>

#include <bitcoin/database/impl/tables/hash_table.ipp>

#undef CLASS
#undef TEMPLATE

#endif
