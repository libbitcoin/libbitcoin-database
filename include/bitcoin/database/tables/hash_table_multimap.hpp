/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_MULTIMAP_HPP
#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_MULTIMAP_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

template <typename Index>
class hash_table_multimap
{
public:
private:
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Index>
#define CLASS hash_table_multimap<Index>

#include <bitcoin/database/impl/tables/hash_table_multimap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
