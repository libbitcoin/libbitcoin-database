/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
    using header = schema::header::link;
    using hash_map<schema::strong_tx>::hashmap;
    static constexpr auto offset = header::bits;
    static_assert(offset < to_bits(header::size));

    static constexpr header::integer merge(bool positive,
        header::integer header_fk) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT_MSG(!get_right(header_fk, offset), "overflow");
        return set_right(header_fk, offset, positive);
    }

    struct record
      : public schema::strong_tx
    {
        inline bool positive() const NOEXCEPT
        {
            return system::get_right(signed_block_fk, offset);
        }

        inline header::integer header_fk() const NOEXCEPT
        {
            return system::set_right(signed_block_fk, offset, false);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            signed_block_fk = source.read_little_endian<header::integer, header::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<header::integer, header::size>(signed_block_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return positive() == other.positive()
                && header_fk() == other.header_fk();
        }

        header::integer signed_block_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
