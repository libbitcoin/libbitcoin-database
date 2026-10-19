/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_STRONG_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_STRONG_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// strong is a head-only array of signed header fks indexed by tx.fk.
/// A zero value (the fill) is a negative association to header zero.
struct strong
  : public value_map<schema::strong>
{
    using header = schema::header::link;
    using value = schema::strong::value;
    using value_map<schema::strong>::headmap;
    static constexpr auto offset = header::bits;
    static_assert(offset < to_bits(header::size));

    static constexpr value merge(bool positive,
        header::integer header_fk) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT_MSG(!get_right(header_fk, offset), "overflow");
        return set_right(header_fk, offset, positive);
    }

    static constexpr bool positive(value signed_block_fk) NOEXCEPT
    {
        return system::get_right(signed_block_fk, offset);
    }

    static constexpr header::integer header_fk(value signed_block_fk) NOEXCEPT
    {
        return system::set_right(signed_block_fk, offset, false);
    }

    /// Replace the association.
    struct replace
      : public schema::strong
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(cell);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<value, cell>(signed_block_fk);
            return sink;
        }

        value signed_block_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
