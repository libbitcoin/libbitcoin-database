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
struct spend
  : public hash_map<schema::spend>
{
    using tx = linkage<schema::tx>;
    using ix = linkage<schema::index>;
    using hash_map<schema::spend>::hashmap;
    using search_key = search<schema::spend::sk>;

    // Composers/decomposers do not adjust to type changes.
    static_assert(tx::size == 4 && ix::size == 3);

    static constexpr search_key compose(tx::integer fk,
        ix::integer index) NOEXCEPT
    {
        return
        {
            system::byte<0>(fk),
            system::byte<1>(fk),
            system::byte<2>(fk),
            system::byte<3>(fk),
            system::byte<0>(index),
            system::byte<1>(index),
            system::byte<2>(index)
        };
    }

    static CONSTEVAL search_key null_point() NOEXCEPT
    {
        return compose(tx::terminal, ix::terminal);
    };

    static inline tx decompose_fk(const search_key& key) NOEXCEPT
    {
        return system::array_cast<uint8_t, tx::size>(key);
    }

    static inline ix decompose_index(const search_key& key) NOEXCEPT
    {
        return system::array_cast<uint8_t, ix::size, tx::size>(key);
    }

    struct record
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            tx_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(tx_fk);
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return tx_fk == other.tx_fk;
        }

        tx::integer tx_fk{};
    };

    struct record_composite_sk
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_forward<sk>();
            return source;
        }

        inline tx point_fk() const NOEXCEPT
        {
            return decompose_fk(key);
        }

        inline ix point_index() const NOEXCEPT
        {
            return decompose_index(key);
        }

        inline bool is_null() const NOEXCEPT
        {
            return key == null_point();
        }

        search_key key{};
    };

    struct record_decomposed_fk
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return point_fk == tx::terminal;
        }

        tx::integer point_fk{};
    };

    struct record_decomposed_sk
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk    = source.read_little_endian<tx::integer, tx::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            // Restore truncated null_index sentinel. Foreign point index is
            // limited to 3 bytes, which cannot hold null_index. Sentinel 
            // 0xffffffff truncated to 0x00ffffff upon write and explicitly
            // restored to 0xffffffff upon read.
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return point_fk == tx::terminal;
        }

        tx::integer point_fk{};
        ix::integer point_index{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
