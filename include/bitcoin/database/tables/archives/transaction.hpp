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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TRANSACTION_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TRANSACTION_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Transaction is a canonical record hash table.
struct transaction
  : public hash_map<schema::transaction>
{
    using ix = linkage<schema::index>;
    using ins = linkage<schema::ins_>;
    using outs = linkage<schema::outs_>;
    using out = linkage<schema::put>;
    using bytes = linkage<schema::size>;
    using search_key = search<schema::hash>;
    using hash_map<schema::transaction>::hashmap;

    static constexpr size_t skip_to_version =
        schema::bit +
        bytes::size +
        bytes::size +
        sizeof(uint32_t);

    static constexpr size_t skip_to_outs =
        skip_to_version +
        sizeof(uint32_t);

    struct record
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            coinbase   = to_bool(source.read_byte());
            light      = source.read_little_endian<bytes::integer, bytes::size>();
            heavy      = source.read_little_endian<bytes::integer, bytes::size>();
            locktime   = source.read_little_endian<uint32_t>();
            version    = source.read_little_endian<uint32_t>();
            ins_count  = source.read_little_endian<ix::integer, ix::size>();
            outs_count = source.read_little_endian<ix::integer, ix::size>();
            point_fk   = source.read_little_endian<ins::integer, ins::size>();
            outs_fk    = source.read_little_endian<outs::integer, outs::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_byte(to_int<uint8_t>(coinbase));
            sink.write_little_endian<bytes::integer, bytes::size>(light);
            sink.write_little_endian<bytes::integer, bytes::size>(heavy);
            sink.write_little_endian<uint32_t>(locktime);
            sink.write_little_endian<uint32_t>(version);
            sink.write_little_endian<ix::integer, ix::size>(ins_count);
            sink.write_little_endian<ix::integer, ix::size>(outs_count);
            sink.write_little_endian<ins::integer, ins::size>(point_fk);
            sink.write_little_endian<outs::integer, outs::size>(outs_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return coinbase == other.coinbase
                && light == other.light
                && heavy == other.heavy
                && locktime == other.locktime
                && version == other.version
                && ins_count == other.ins_count
                && outs_count == other.outs_count
                && point_fk == other.point_fk
                && outs_fk == other.outs_fk;
        }

        bool coinbase{};
        bytes::integer light{}; // tx.serialized_size(false)
        bytes::integer heavy{}; // tx.serialized_size(true)
        uint32_t locktime{};
        uint32_t version{};
        ix::integer ins_count{};
        ix::integer outs_count{};
        ins::integer point_fk{};
        outs::integer outs_fk{};
    };

    struct only
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            static constexpr size_t skip_size =
                schema::bit +
                bytes::size +
                bytes::size;

            source.skip_bytes(skip_size);
            locktime   = source.read_little_endian<uint32_t>();
            version    = source.read_little_endian<uint32_t>();
            ins_count  = source.read_little_endian<ix::integer, ix::size>();
            outs_count = source.read_little_endian<ix::integer, ix::size>();
            point_fk   = source.read_little_endian<ins::integer, ins::size>();
            outs_fk    = source.read_little_endian<outs::integer, outs::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        uint32_t locktime{};
        uint32_t version{};
        ix::integer ins_count{};
        ix::integer outs_count{};
        ins::integer point_fk{};
        outs::integer outs_fk{};
    };

    struct record_put_ref
      : public schema::transaction
    {
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            using namespace system;
            sink.write_byte(to_int<uint8_t>(tx.is_coinbase()));
            sink.write_little_endian<bytes::integer, bytes::size>(
                possible_narrow_cast<bytes::integer>(tx.serialized_size(false)));
            sink.write_little_endian<bytes::integer, bytes::size>(
                possible_narrow_cast<bytes::integer>(tx.serialized_size(true)));
            sink.write_little_endian<uint32_t>(tx.locktime());
            sink.write_little_endian<uint32_t>(tx.version());
            sink.write_little_endian<ix::integer, ix::size>(ins_count);
            sink.write_little_endian<ix::integer, ix::size>(outs_count);
            sink.write_little_endian<ins::integer, ins::size>(point_fk);
            sink.write_little_endian<outs::integer, outs::size>(outs_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::chain::transaction& tx{};
        ix::integer ins_count{};
        ix::integer outs_count{};
        ins::integer point_fk{};
        outs::integer outs_fk{};
    };

    struct only_with_sk
      : public only
    {
        BC_PUSH_WARNING(NO_METHOD_HIDING)
        inline bool from_data(reader& source) NOEXCEPT
        BC_POP_WARNING()
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            return only::from_data(source);
        }

        search_key key{};
    };

    struct get_put_counts
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_outs);
            ins_count  = source.read_little_endian<ix::integer, ix::size>();
            outs_count = source.read_little_endian<ix::integer, ix::size>();
            return source;
        }

        ix::integer ins_count{};
        ix::integer outs_count{};
    };

    struct get_outs
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_outs);
            ins_count  = source.read_little_endian<ix::integer, ix::size>();
            outs_count = source.read_little_endian<ix::integer, ix::size>();
            point_fk   = source.read_little_endian<ins::integer, ins::size>();
            outs_fk    = source.read_little_endian<outs::integer, outs::size>();
            return source;
        }

        ix::integer ins_count{};
        ix::integer outs_count{};
        ins::integer point_fk{};
        outs::integer outs_fk{};
    };

    struct get_version
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_version);
            version = source.read_little_endian<uint32_t>();
            return source;
        }

        uint32_t version{};
    };

    struct get_set_ref
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_version);
            set.version = source.read_little_endian<uint32_t>();
            set.points.resize(source.read_little_endian<ix::integer, ix::size>());
            return source;
        }

        point_set& set;
    };

    struct get_point
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_outs);
            number = source.read_little_endian<ix::integer, ix::size>();

            if (index >= number)
            {
                point_fk = ins::terminal;
                return source;
            }

            source.skip_bytes(ix::size);
            point_fk = source.read_little_endian<ins::integer, ins::size>() + index;
            return source;
        }

        // Index provides optional offset for point_fk, number is absolute.
        const ins::integer index{};
        ins::integer point_fk{};
        ix::integer number{};
    };

    struct get_output
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_outs + ix::size);
            number = source.read_little_endian<ix::integer, ix::size>();

            if (index >= number)
            {
                outs_fk = outs::terminal;
                return source;
            }

            source.skip_bytes(ins::size);
            outs_fk = source.read_little_endian<outs::integer, outs::size>() + index;
            return source;
        }

        // Index provides optional offset for outs_fk, number is absolute.
        const outs::integer index{};
        outs::integer outs_fk{};
        ix::integer number{};
    };

    struct get_coinbase
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            coinbase = to_bool(source.read_byte());
            return source;
        }

        bool coinbase{};
    };

    struct get_sizes
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_byte();
            light = source.read_little_endian<bytes::integer, bytes::size>();
            heavy = source.read_little_endian<bytes::integer, bytes::size>();
            return source;
        }

        size_t light{};
        size_t heavy{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
