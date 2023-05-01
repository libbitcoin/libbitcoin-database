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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_STRONG_TX_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_STRONG_TX_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// strong_tx is a record hashmap of tx confirmation state.
struct strong_tx
  : public hash_map<schema::strong_tx>
{
    using block = linkage<schema::block>;
    using hash_map<schema::strong_tx>::hashmap;

    struct record
      : public schema::strong_tx
    {
        template <typename Reader>
        inline bool from_data(Reader& source) NOEXCEPT
        {
            header_fk = source.template read_little_endian<block::integer, block::size>();
            return source;
        }

        template <typename Writer>
        inline bool to_data(Writer& sink) const NOEXCEPT
        {
            sink.template write_little_endian<block::integer, block::size>(header_fk);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return header_fk == other.header_fk;
        }

        block::integer header_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
