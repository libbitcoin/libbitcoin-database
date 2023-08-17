/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_SPEND_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_SPEND_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Spend is a record multimap of transactions by output(s) spent.
/// Sequence is part of an input, denormalized here for confirmation.
struct spend
  : public hash_map<schema::spend>
{
    using tx = linkage<schema::tx>;
    using ix = linkage<schema::index>;
    using pt = linkage<schema::point::pk>;
    using in = linkage<schema::input::pk>;
    using hash_map<schema::spend>::hashmap;
    using search_key = search<schema::spend::sk>;

    // Composers/decomposers do not adjust to type changes.
    static_assert(pt::size == 4 && ix::size == 3);

    static constexpr search_key compose(pt::integer point_fk,
        ix::integer point_index) NOEXCEPT
    {
        return
        {
            system::byte<0>(point_fk),
            system::byte<1>(point_fk),
            system::byte<2>(point_fk),
            system::byte<3>(point_fk),
            system::byte<0>(point_index),
            system::byte<1>(point_index),
            system::byte<2>(point_index)
        };
    }

    struct record
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_little_endian<uint32_t>(sequence);
            sink.write_little_endian<in::integer, in::size>(input_fk);
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return parent_fk == other.parent_fk
                && sequence == other.sequence
                && input_fk == other.input_fk;
        }

        tx::integer parent_fk{};
        uint32_t sequence{};
        in::integer input_fk{};
    };

    struct get_input
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<pt::integer, pt::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            if (null_point(point_fk))
                point_index = system::chain::point::null_index;

            source.skip_bytes(tx::size);
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_fk);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        uint32_t sequence{};
        in::integer input_fk{};
    };

    struct get_parent
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        tx::integer parent_fk{};
    };

    struct get_point
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<pt::integer, pt::size>();
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_fk);
        }

        pt::integer point_fk{};
    };

    struct get_key
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_forward<sk>();
            return source;
        }

        search_key key{};
    };

    struct get_prevout
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<pt::integer, pt::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            if (null_point(point_fk))
                point_index = system::chain::point::null_index;

            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_fk);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
    };

    struct get_prevout_parent
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<pt::integer, pt::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            if (null_point(point_fk))
                point_index = system::chain::point::null_index;

            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        inline search_key prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_fk);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        tx::integer parent_fk{};
    };

    struct get_prevout_parent_sequence
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<pt::integer, pt::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            if (null_point(point_fk))
                point_index = system::chain::point::null_index;

            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            sequence = source.read_little_endian<uint32_t>();
            return source;
        }

        inline search_key prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_fk);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        tx::integer parent_fk{};
        uint32_t sequence{};
    };

private:
    static constexpr bool null_point(pt::integer fk) NOEXCEPT
    {
        return fk == pt::terminal;
    }
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
