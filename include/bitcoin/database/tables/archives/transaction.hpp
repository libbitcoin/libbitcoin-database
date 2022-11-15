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

BC_PUSH_WARNING(NO_METHOD_HIDING)

// Transaction is a cononical record hash table.

struct record
{
    // Sizes.
    static constexpr size_t pk = schema::tx;
    static constexpr size_t sk = schema::hash;
    static constexpr size_t minsize =
        schema::code +
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

    // Fields.
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

    // Serialializers.

    inline record from_data(reader& source) NOEXCEPT
    {
        coinbase   = to_bool(source.read_byte());
        bytes      = source.read_3_bytes_little_endian();
        weight     = source.read_3_bytes_little_endian();
        locktime   = source.read_4_bytes_little_endian();
        version    = source.read_4_bytes_little_endian();
        ins_count  = source.read_3_bytes_little_endian();
        ins_fk     = source.read_4_bytes_little_endian();
        outs_count = source.read_3_bytes_little_endian();
        outs_fk    = source.read_4_bytes_little_endian();
        BC_ASSERT(source.get_position() == minrow);
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        sink.write_byte(to_int<uint8_t>(coinbase));
        sink.write_3_bytes_little_endian(bytes);
        sink.write_3_bytes_little_endian(weight);
        sink.write_4_bytes_little_endian(locktime);
        sink.write_4_bytes_little_endian(version);
        sink.write_3_bytes_little_endian(ins_count);
        sink.write_4_bytes_little_endian(ins_fk);
        sink.write_3_bytes_little_endian(outs_count);
        sink.write_4_bytes_little_endian(outs_fk);
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

BC_POP_WARNING()

} // namespace transaction
} // namespace database
} // namespace libbitcoin

#endif
