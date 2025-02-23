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
  : public hash_map<schema::point>
{
    using hash_map<schema::point>::hashmap;
    using search_key = search<schema::point::sk>;
    using ix = linkage<schema::index>;

    static inline search_key compose(const hash_digest& point_hash,
        ix::integer point_index) NOEXCEPT
    {
        search_key key{};
        system::array_cast<uint8_t, schema::hash>(key) = point_hash;
        key.at(schema::hash + 0) = system::byte<0>(point_index);
        key.at(schema::hash + 1) = system::byte<1>(point_index);
        key.at(schema::hash + 2) = system::byte<2>(point_index);
        return key;
    }

    static search_key compose(const system::chain::point& point) NOEXCEPT
    {
        return compose(point.hash(), point.index());
    }

    struct record
      : public schema::point
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(schema::point::sk);
            hash = source.read_hash();
            index = source.read_little_endian<ix::integer, ix::size>();

            if (index == ix::terminal)
                index = system::chain::point::null_index;

            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool is_null() const NOEXCEPT
        {
            return index == system::chain::point::null_index;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return hash == other.hash
                && index == other.index;
        }

        hash_digest hash{};
        ix::integer index{};
    };

    struct get_composed
      : public schema::point
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(schema::point::sk);
            key = source.read_forward<schema::point::sk>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        search_key key{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
