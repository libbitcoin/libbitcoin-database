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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_STATE_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_STATE_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// state is a head-only array of block state flags, indexed by header.fk.
struct state
  : public value_map<schema::state>
{
    using value_map<schema::state>::headmap;

    /// Merge flags into the current state.
    struct merge
      : public schema::state
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            current = source.read_byte();
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_byte(system::bit_or<value>(current, flags));
            return sink;
        }

        value flags{};
        value current{};
    };

    /// Replace the current state.
    struct replace
      : public schema::state
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_byte();
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_byte(flags);
            return sink;
        }

        value flags{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
