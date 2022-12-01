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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Point records are empty, providing only a sk<->fk compression mapping.
/// Each record is 32+4=36 bytes, enabling 4 byte point.hash storage.
class point
  : public hash_map<schema::point>
{
public:
    using search_key = search<schema::hash>;

    using hash_map<schema::point>::hashmap;

    struct record
      : public schema::point
    {
        inline bool from_data(const reader& source) NOEXCEPT
        {
            // debug warning if source non-const, but get_position is non-const.
            ////BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(const finalizer& sink) const NOEXCEPT
        {
            // debug warning if sink non-const, but get_position is non-const.
            ////BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT
        {
            return true;
        }
    };

    struct record_sk
      : public schema::point
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            return source;
        }

        search_key key{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
