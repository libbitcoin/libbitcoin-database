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
namespace transaction {

/// Transaction is a cononical record hash table.
    
// TODO: coinbase bit can be merged into bytes field (saving ~.7GB).
struct record
{
    /// Sizes.
    static constexpr size_t pk = schema::tx;
    static constexpr size_t sk = schema::hash;
    static constexpr size_t minsize =
        schema::bit +
        schema::size +
        schema::size +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        schema::index +
        schema::index +
        schema::puts;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static_assert(minsize == 25u);
    static_assert(minrow == 61u);

    static constexpr linkage<pk> count() NOEXCEPT { return 1; }

    /// Fields.
    bool coinbase{};
    uint32_t bytes{};
    uint32_t weight{};
    uint32_t locktime{};
    uint32_t version{};
    uint32_t ins_count{};
    uint32_t outs_count{};
    uint32_t ins_fk{};

    /// Computed outputs start is based on presumed txs table schema.
    inline uint32_t outs_fk() const NOEXCEPT
    {
        return ins_fk + ins_count * schema::put;
    }

    /// Serialializers.

    inline bool from_data(reader& source) NOEXCEPT
    {
        coinbase   = to_bool(source.read_byte());
        bytes      = source.read_little_endian<uint32_t, schema::size>();
        weight     = source.read_little_endian<uint32_t, schema::size>();
        locktime   = source.read_little_endian<uint32_t>();
        version    = source.read_little_endian<uint32_t>();
        ins_count  = source.read_little_endian<uint32_t, schema::index>();
        outs_count = source.read_little_endian<uint32_t, schema::index>();
        ins_fk     = source.read_little_endian<uint32_t, schema::puts>();
        BC_ASSERT(source.get_position() == minrow);
        return source;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        sink.write_byte(to_int<uint8_t>(coinbase));
        sink.write_little_endian<uint32_t, schema::size>(bytes);
        sink.write_little_endian<uint32_t, schema::size>(weight);
        sink.write_little_endian<uint32_t>(locktime);
        sink.write_little_endian<uint32_t>(version);
        sink.write_little_endian<uint32_t, schema::index>(ins_count);
        sink.write_little_endian<uint32_t, schema::index>(outs_count);
        sink.write_little_endian<uint32_t, schema::puts>(ins_fk);
        BC_ASSERT(sink.get_position() == minrow);
        return sink;
    }

    inline bool operator==(const record& other) const NOEXCEPT
    {
        return coinbase == other.coinbase
            && bytes == other.bytes
            && weight == other.weight
            && locktime == other.locktime
            && version == other.version
            && ins_count == other.ins_count
            && outs_count == other.outs_count
            && ins_fk == other.ins_fk;
    }
};

/// Get search key only (this is generic for all hash tables).
struct record_sk
{
    static constexpr size_t size = record::size;

    inline bool from_data(reader& source) NOEXCEPT
    {
        source.rewind_bytes(record::sk);
        sk = source.read_hash();
        return source;
    }

    hash_digest sk{};
};

/// Get puts only.
struct record_puts
{
    static constexpr size_t size = record::size;

    /// Computed outputs start is based on presumed txs table schema.
    inline uint32_t outs_fk() const NOEXCEPT
    {
        return ins_fk + ins_count * schema::put;
    }

    inline bool from_data(reader& source) NOEXCEPT
    {
        static constexpr size_t skip_size =
            schema::bit +
            schema::size +
            schema::size +
            sizeof(uint32_t) +
            sizeof(uint32_t);

        source.skip_bytes(skip_size);
        ins_count  = source.read_little_endian<uint32_t, schema::index>();
        outs_count = source.read_little_endian<uint32_t, schema::index>();
        ins_fk     = source.read_little_endian<uint32_t, schema::puts>();
        return source;
    }

    uint32_t ins_count{};
    uint32_t outs_count{};
    uint32_t ins_fk{};
};

/// transaction::table
class BCD_API table
  : public hash_map<record>
{
public:
    using hash_map<record>::hashmap;
};

} // namespace transaction
} // namespace database
} // namespace libbitcoin

#endif
