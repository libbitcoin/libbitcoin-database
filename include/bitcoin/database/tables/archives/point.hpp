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

/// Sequence is part of an input, denormalized here for confirmation.
struct point
  : public no_map<schema::point>
{
    using no_map<schema::point>::nomap;
    using point_stub = linkage<schema::tx>;
    using pt = linkage<schema::point_>;
    using ix = linkage<schema::index>;
    using in = linkage<schema::put>;
    using tx = linkage<schema::tx>;

    struct record
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            index = source.read_little_endian<ix::integer, ix::size>();
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            parent_fk = source.read_little_endian<tx::integer, tx::size>();

            if (index == ix::terminal)
                index = system::chain::point::null_index;

            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(hash);
            sink.write_little_endian<ix::integer, ix::size>(index);
            sink.write_little_endian<uint32_t>(sequence);
            sink.write_little_endian<in::integer, in::size>(input_fk);
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
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
                && index == other.index
                && sequence == other.sequence
                && input_fk == other.input_fk
                && parent_fk == other.parent_fk;
        }

        hash_digest hash{};
        ix::integer index{};
        uint32_t sequence{};
        in::integer input_fk{};
        tx::integer parent_fk{};
    };

    struct get_input
        : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            index = source.read_little_endian<ix::integer, ix::size>();
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();

            if (index == ix::terminal)
                index = system::chain::point::null_index;

            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return index == system::chain::point::null_index;
        }

        hash_digest hash{};
        ix::integer index{};
        uint32_t sequence{};
        in::integer input_fk{};
    };

    struct get_parent_key
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            source.skip_bytes(ix::size + sizeof(uint32_t) + in::size);
            fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        hash_digest hash{};
        tx::integer fk{};
    };

    struct get_parent
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(schema::hash + ix::size + sizeof(uint32_t) + in::size);
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        tx::integer parent_fk{};
    };

    struct get_point
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            index = source.read_little_endian<ix::integer, ix::size>();

            if (index == ix::terminal)
                index = system::chain::point::null_index;

            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return index == system::chain::point::null_index;
        }

        hash_digest hash{};
        ix::integer index{};
    };


    struct get_key
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            hash = source.read_hash();
            return source;
        }

        hash_digest hash{};
    };

    struct get_stub
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        using ps = point_stub;
        inline bool from_data(reader& source) NOEXCEPT
        {
            stub = source.read_little_endian<ps::integer, ps::size>();
            return source;
        }

        ps::integer stub{};
    };

    struct get_spend_key
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            return one;
        }

        using ps = point_stub;
        inline bool from_data(reader& source) NOEXCEPT
        {
            stub = source.read_little_endian<ps::integer, ps::size>();
            source.skip_bytes(schema::hash - ps::size);
            index = source.read_little_endian<ix::integer, ix::size>();
            return source;
        }

        ps::integer stub{};
        ix::integer index{};
    };

    struct get_point_set_ref
      : public schema::point
    {
        inline link count() const NOEXCEPT
        {
            const auto points = set.points.size();
            BC_ASSERT(points < link::terminal);
            return system::possible_narrow_cast<link::integer>(points);
        }

        using ps = point_stub;
        inline bool from_data(reader& source) NOEXCEPT
        {
            pt::integer offset{};
            std::for_each(set.points.begin(), set.points.end(), [&](auto& point) NOEXCEPT
            {
                point.self     = set.fk + offset++;
                point.stub     = source.read_little_endian<ps::integer, ps::size>();
                source.skip_bytes(schema::hash - ps::size);
                point.index    = source.read_little_endian<ix::integer, ix::size>();
                point.sequence = source.read_little_endian<uint32_t>();
                source.skip_bytes(ix::size + tx::size);
            });

            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        point_set& set;
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
