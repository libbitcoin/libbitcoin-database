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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_SPEND_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_SPEND_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Spend is a record multimap of inputs indexed by output spent.
/// Sequence is part of an input, denormalized here for confirmation.
struct spend
  : public hash_map<schema::spend>
{
    using tx = linkage<schema::tx>;
    using ix = linkage<schema::index>;
    using in = linkage<schema::input::pk>;
    using hash_map<schema::spend>::hashmap;
    using search_key = search<schema::spend::sk>;

    static constexpr bool null_point(ix::integer index) NOEXCEPT
    {
        return index == system::chain::point::null_index;
    }

    static inline search_key compose(const hash_digest& point_hash,
        ix::integer point_index) NOEXCEPT
    {
        using namespace system;

        search_key key{};
        array_cast<uint8_t, schema::hash>(key) = point_hash;
        key.at(schema::hash + 0) = byte<0>(point_index);
        key.at(schema::hash + 1) = byte<1>(point_index);
        key.at(schema::hash + 2) = byte<2>(point_index);
        return key;
    }

    struct record
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_little_endian<uint32_t>(sequence);
            sink.write_little_endian<in::integer, in::size>(input_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
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
            point_hash = source.read_hash();
            point_index = source.read_little_endian<ix::integer, ix::size>();
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            source.skip_bytes(tx::size);
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_index);
        }

        hash_digest point_hash{};
        ix::integer point_index{};
        uint32_t sequence{};
        in::integer input_fk{};
    };

    struct get_point
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_hash = source.read_hash();
            point_index = source.read_little_endian<ix::integer, ix::size>();
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_index);
        }

        hash_digest point_hash{};
        ix::integer point_index{};
    };

    struct get_point_hash
      : public schema::spend
    {
        // Could read index and check for terminal before hash read. But that
        // is an optimization for coinbase txs, which may not be called here.
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_hash = source.read_hash();
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return point_hash == system::null_hash;
        }

        hash_digest point_hash{};
    };

    struct get_point_parent
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_hash = source.read_hash();
            point_index = source.read_little_endian<ix::integer, ix::size>();
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_index);
        }

        hash_digest point_hash{};
        ix::integer point_index{};
        tx::integer parent_fk{};
    };

    struct get_point_sequence
      : public schema::spend
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_hash = source.read_hash();
            point_index = source.read_little_endian<ix::integer, ix::size>();
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            source.skip_bytes(tx::size);
            sequence = source.read_little_endian<uint32_t>();
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_point(point_index);
        }

        hash_digest point_hash{};
        ix::integer point_index{};
        uint32_t sequence{};
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

private:
    void index_guard() const NOEXCEPT
    {
        using ix = linkage<schema::index>;

        // Composition does not adjust to change to index size.
        static_assert(ix::size == 3);

        // Ensure null_index truncates to terminal.
        static constexpr auto null = system::chain::no_previous_output;
        static constexpr auto mask = byte_bits * (sizeof(null) - ix::size);
        static constexpr auto masked = system::mask_left<ix::integer>(null, mask);
        static_assert(ix::terminal == masked);
    }
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
