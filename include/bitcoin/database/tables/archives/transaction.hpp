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

/// Transaction is a cononical record hash table.
class transaction
  : public hash_map<schema::transaction>
{
public:
    using ix = linkage<schema::index>;
    using put = linkage<schema::put>;
    using puts = linkage<schema::puts_>;
    using bytes = linkage<schema::size>;
    using search_key = search<schema::hash>;

    using hash_map<schema::transaction>::hashmap;

    struct record
      : public schema::transaction
    {
        inline uint32_t outs_fk() const NOEXCEPT
        {
            return ins_fk + ins_count * put::size;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            coinbase   = to_bool(source.read_byte());
            witless    = source.read_little_endian<bytes::integer, bytes::size>();
            witness    = source.read_little_endian<bytes::integer, bytes::size>();
            locktime   = source.read_little_endian<uint32_t>();
            version    = source.read_little_endian<uint32_t>();
            ins_count  = source.read_little_endian<ix::integer, ix::size>();
            outs_count = source.read_little_endian<ix::integer, ix::size>();
            ins_fk     = source.read_little_endian<puts::integer, puts::size>();
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_byte(to_int<uint8_t>(coinbase));
            sink.write_little_endian<bytes::integer, bytes::size>(witless);
            sink.write_little_endian<bytes::integer, bytes::size>(witness);
            sink.write_little_endian<uint32_t>(locktime);
            sink.write_little_endian<uint32_t>(version);
            sink.write_little_endian<ix::integer, ix::size>(ins_count);
            sink.write_little_endian<ix::integer, ix::size>(outs_count);
            sink.write_little_endian<puts::integer, puts::size>(ins_fk);
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return coinbase == other.coinbase
                && witless == other.witless
                && witness == other.witness
                && locktime == other.locktime
                && version == other.version
                && ins_count == other.ins_count
                && outs_count == other.outs_count
                && ins_fk == other.ins_fk;
        }

        bool coinbase{};
        bytes::integer witless{}; // tx.serialized_size(false)
        bytes::integer witness{}; // tx.serialized_size(true)
        uint32_t locktime{};
        uint32_t version{};
        ix::integer ins_count{};
        ix::integer outs_count{};
        puts::integer ins_fk{};
    };

    struct record_sk
      : public schema::transaction
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            return source;
        }

        search_key key{};
    };

    struct record_puts
      : public schema::transaction
    {
        inline uint32_t outs_fk() const NOEXCEPT
        {
            return ins_fk + ins_count * put::size;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            static constexpr size_t skip_size =
                schema::bit +
                bytes::size +
                bytes::size +
                sizeof(uint32_t) +
                sizeof(uint32_t);

            source.skip_bytes(skip_size);
            ins_count  = source.read_little_endian<ix::integer, ix::size>();
            outs_count = source.read_little_endian<ix::integer, ix::size>();
            ins_fk     = source.read_little_endian<puts::integer, puts::size>();
            return source;
        }

        ix::integer ins_count{};
        ix::integer outs_count{};
        puts::integer ins_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
