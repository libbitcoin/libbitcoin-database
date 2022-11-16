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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_INPUT_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_INPUT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace input {

/// Input is searchable by point_fk/index (fP) of the output that it spends.
/// This makes input a multimap, as multiple inputs can spend a given output.

struct slab
{
    /// Sizes.
    static constexpr size_t pk = schema::put;
    static constexpr size_t sk = schema::tx_fp;
    static constexpr size_t minsize =
        schema::tx +
        1u + // variable_size (average 1)
        sizeof(uint32_t) +
        1u + // variable_size (average 1)
        1u;  // variable_size (average 1)
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 11u);
    static_assert(minrow == 23u);

    linkage<pk> count() const NOEXCEPT
    {
        return pk + sk +
            schema::tx +
            variable_size(index) +
            sizeof(uint32_t) +
            script.serialized_size(true) +
            witness.serialized_size(true);
    }

    /// Fields.
    uint32_t parent_fk; // parent fk *is* a required query.
    uint32_t index;     // own (parent-relative) index not a required query.
    uint32_t sequence;
    system::chain::script script;
    system::chain::witness witness;
    bool valid{ false };

    /// Serialializers.

    inline slab from_data(reader& source) NOEXCEPT
    {
        parent_fk = source.read_little_endian<uint32_t, schema::tx>();
        index     = system::narrow_cast<uint32_t>(source.read_variable());
        sequence  = source.read_little_endian<uint32_t>();
        script    = system::chain::script(source, true);
        witness   = system::chain::witness(source, true);
        BC_ASSERT(source.get_position() == count());
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint32_t, schema::tx>(parent_fk);
        sink.write_variable(index);
        sink.write_little_endian<uint32_t>(sequence);
        script.to_data(sink, true);
        witness.to_data(sink, true);
        BC_ASSERT(sink.get_position() == count());
        return sink;
    }
};

/// input::table
class BCD_API table
  : public hash_map<slab>
{
public:
    using hash_map<slab>::hashmap;
};

} // namespace input
} // namespace database
} // namespace libbitcoin

#endif
