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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_DOUBLES_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_DOUBLES_HPP

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

// TODO: this table isn't actually mapped.
struct doubles
  : public hash_map<schema::doubles>
{
    using hash_map<schema::doubles>::hashmap;
    using ix = linkage<schema::index>;

    // This supports only a single record (not too useful).
    struct record
      : public schema::doubles
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            index = source.read_little_endian<ix::integer, ix::size>();
            BC_ASSERT(!source || source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_bytes(hash);
            sink.write_little_endian<ix::integer, ix::size>(index);
            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return hash == other.hash
                && index == other.index;
        }

        hash_digest hash{};
        ix::integer index{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
