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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_STATE_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_STATE_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// state is a record arraymap of block state, indexed by header.fk.
struct state
  : public array_map<schema::state>
{
    using coding = linkage<schema::code>;
    using array_map<schema::state>::arraymap;

    struct record
      : public schema::state
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            code = source.read_little_endian<coding::integer, coding::size>();
            BC_ASSERT(!source || source.get_read_position() == count() * minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<coding::integer, coding::size>(code);
            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        coding::integer code{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
