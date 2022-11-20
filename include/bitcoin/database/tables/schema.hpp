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
#ifndef LIBBITCOIN_DATABASE_TABLES_SCHEMA_HPP
#define LIBBITCOIN_DATABASE_TABLES_SCHEMA_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

template <size_t Size>
using search = system::data_array<Size>;
using hash_digest = system::hash_digest;

namespace schema
{
    constexpr size_t block = 3;
    constexpr size_t tx = 4;
    constexpr size_t txs = 4;
    constexpr size_t puts = 4;
    constexpr size_t put = 5;
    constexpr size_t bit = 1;
    constexpr size_t code = 1;
    constexpr size_t size = 3;
    constexpr size_t index = 3;
    constexpr size_t sigops = 3;
    constexpr size_t flags = 4;
    constexpr size_t tx_fp = tx + index;
    constexpr size_t hash = system::hash_size;
}

} // namespace database
} // namespace libbitcoin

#endif
