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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace point {

BC_PUSH_WARNING(NO_METHOD_HIDING)

// Point records are empty, providing only a sk<->fk compression mapping.
// Each record is 32+4=36 bytes, enabling 4 byte point.hash storage.

struct record
{
    // Sizes.
    static constexpr size_t pk = schema::tx;
    static constexpr size_t sk = schema::hash;
    static constexpr size_t minsize = zero;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static_assert(minsize == 0u);
    static_assert(minrow == 36u);

    static constexpr linkage<pk> count() NOEXCEPT { return 1; }

    // Fields.
    bool valid{ false };

    // Serialializers.

    inline record from_data(reader& source) NOEXCEPT
    {
        BC_ASSERT(source.get_position() == minrow);
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        BC_ASSERT(sink.get_position() == minrow);
        return sink;
    }
};

struct record_sk
{
    static constexpr size_t size = record::size;

    inline record_sk from_data(reader& source) NOEXCEPT
    {
        source.rewind_bytes(record::sk);
        sk = source.read_hash();
        valid = source;
        return *this;
    }

    hash_digest sk;
    bool valid{ false };
};

struct record_pk
{
    inline record_pk from_data(reader& source) NOEXCEPT
    {
        source.rewind_bytes(record::pk + record::sk);
        pk = source.read_4_bytes_little_endian();
        valid = source;
        return *this;
    }

    uint32_t pk;
    bool valid{ false };
};

/// point::table
class BCD_API table
  : public hash_map<record>
{
public:
    using hash_map<record>::hashmap;
};

BC_POP_WARNING()

} // namespace point
} // namespace database
} // namespace libbitcoin

#endif
