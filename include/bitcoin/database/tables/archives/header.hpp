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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace header {

/// Header is a cononical record hash table.

struct record
{
    /// Sizes.
    static constexpr size_t pk = schema::block;
    static constexpr size_t sk = schema::hash;
    static constexpr size_t minsize =
        schema::block +
        schema::flags +
        sizeof(uint32_t) +
        pk +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        schema::hash;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static_assert(minsize == 62u);
    static_assert(minrow == 97u);

    static constexpr linkage<pk> count() NOEXCEPT { return 1; }

    /// Fields.
    uint32_t height;
    uint32_t flags;
    uint32_t mtp;
    uint32_t parent_fk;
    uint32_t version;
    uint32_t time;
    uint32_t bits;
    uint32_t nonce;
    hash_digest root;
    bool valid{ false };

    /// Serialializers.

    inline record from_data(reader& source) NOEXCEPT
    {
        height    = source.read_little_endian<uint32_t, schema::block>();
        flags     = source.read_little_endian<uint32_t, schema::flags>();
        mtp       = source.read_little_endian<uint32_t>();
        parent_fk = source.read_little_endian<uint32_t, schema::block>();
        version   = source.read_little_endian<uint32_t>();
        time      = source.read_little_endian<uint32_t>();
        bits      = source.read_little_endian<uint32_t>();
        nonce     = source.read_little_endian<uint32_t>();
        root      = source.read_hash();
        BC_ASSERT(source.get_position() == minrow);
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint32_t, schema::block>(height);
        sink.write_little_endian<uint32_t, schema::flags>(flags);
        sink.write_little_endian<uint32_t>(mtp);
        sink.write_little_endian<uint32_t, schema::block>(parent_fk);
        sink.write_little_endian<uint32_t>(version);
        sink.write_little_endian<uint32_t>(time);
        sink.write_little_endian<uint32_t>(bits);
        sink.write_little_endian<uint32_t>(nonce);
        sink.write_bytes(root);
        BC_ASSERT(sink.get_position() == minrow);
        return sink;
    }

    inline bool operator==(const record& other) const NOEXCEPT
    {
        return valid == other.valid
            && height == other.height
            && flags == other.flags
            && mtp == other.mtp
            && parent_fk == other.parent_fk
            && version == other.version
            && time == other.time
            && bits == other.bits
            && nonce == other.nonce
            && root == other.root;
    }
};

/// Get height only (demo).
struct record_height
{
    static constexpr size_t size = record::size;

    inline record_height from_data(reader& source) NOEXCEPT
    {
        height = source.read_little_endian<uint32_t, schema::block>();
        valid = source;
        return *this;
    }

    uint32_t height;
    bool valid{ false };
};

/// Get record with search key (demo).
struct record_with_key
  : public record
{
    BC_PUSH_WARNING(NO_METHOD_HIDING)
    inline record_with_key from_data(reader& source) NOEXCEPT
    {
        source.rewind_bytes(record::sk);
        key = source.read_hash();
        record::from_data(source);
        return *this;
    }
    BC_POP_WARNING()

    hash_digest key;
};

/// header::table
class BCD_API table
  : public hash_map<record>
{
public:
    using hash_map<record>::hashmap;
};

} // namespace header
} // namespace database
} // namespace libbitcoin

#endif
