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
        schema::puts +
        schema::index +
        schema::puts;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static_assert(minsize == 29u);
    static_assert(minrow == 65u);

    static constexpr linkage<pk> count() NOEXCEPT { return 1; }

    /// Fields.
    bool coinbase;
    uint32_t bytes;
    uint32_t weight;
    uint32_t locktime;
    uint32_t version;
    uint32_t ins_count;
    uint32_t ins_fk;
    uint32_t outs_count;
    uint32_t outs_fk;
    bool valid{ false };

    /// Serialializers.

    inline record from_data(reader& source) NOEXCEPT
    {
        // TODO: coinbase bit can be merged into bytes field (.7GB).
        coinbase   = to_bool(source.read_byte());
        bytes      = source.read_little_endian<uint32_t, schema::size>();
        weight     = source.read_little_endian<uint32_t, schema::size>();
        locktime   = source.read_little_endian<uint32_t>();
        version    = source.read_little_endian<uint32_t>();
        ins_count  = source.read_little_endian<uint32_t, schema::index>();
        ins_fk     = source.read_little_endian<uint32_t, schema::puts>();
        outs_count = source.read_little_endian<uint32_t, schema::index>();
        outs_fk    = source.read_little_endian<uint32_t, schema::puts>();
        BC_ASSERT(source.get_position() == minrow);
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        // TODO: coinbase bit can be merged into bytes field (.7GB).
        sink.write_byte(to_int<uint8_t>(coinbase));
        sink.write_little_endian<uint32_t, schema::size>(bytes);
        sink.write_little_endian<uint32_t, schema::size>(weight);
        sink.write_little_endian<uint32_t>(locktime);
        sink.write_little_endian<uint32_t>(version);
        sink.write_little_endian<uint32_t, schema::index>(ins_count);
        sink.write_little_endian<uint32_t, schema::puts>(ins_fk);
        sink.write_little_endian<uint32_t, schema::index>(outs_count);
        sink.write_little_endian<uint32_t, schema::puts>(outs_fk);
        BC_ASSERT(sink.get_position() == minrow);
        return sink;
    }

    inline bool operator==(const record& other) const NOEXCEPT
    {
        return valid == other.valid
            && coinbase == other.coinbase
            && bytes == other.bytes
            && weight == other.weight
            && locktime == other.locktime
            && version == other.version
            && ins_count == other.ins_count
            && ins_fk == other.ins_fk
            && outs_count == other.outs_count
            && outs_fk == other.outs_fk;
    }
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
