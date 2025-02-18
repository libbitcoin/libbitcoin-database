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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

struct point
  : public no_map<schema::point>
{
    using no_map<schema::point>::nomap;
    using point_stub = linkage<schema::tx>;

    struct record
      : public schema::point
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(hash);
            return sink;
        }

        inline bool is_null() const NOEXCEPT
        {
            return hash == system::null_hash;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return hash == other.hash;
        }

        hash_digest hash{};
    };

    struct record_ref
      : public schema::point
    {
        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(hash);
            return sink;
        }

        const hash_digest& hash{};
    };

    // point
    struct get_stub
      : public schema::point
    {
        using ps = point_stub;
        inline bool from_data(reader& source) NOEXCEPT
        {
            stub = source.read_little_endian<ps::integer, ps::size>();
            return source;
        }
    
        ps::integer stub{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
