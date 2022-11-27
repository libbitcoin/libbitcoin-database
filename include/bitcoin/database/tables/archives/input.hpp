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
namespace table {

/// Input is searchable by point_fk/index (fP) of the output that it spends.
/// This makes input a multimap, as multiple inputs can spend a given output.
class input
  : public hash_map<schema::input>
{
public:
    using hash_map<schema::input>::hashmap;

    /// Generate composite key.
    static const search<schema::input::sk> to_point(uint32_t fk,
        uint32_t index) NOEXCEPT
    {
        // TODO: generalize/optimize.
        search<schema::input::sk> value{};
        system::write::bytes::copy sink(value);
        sink.write_little_endian<uint32_t, schema::tx>(fk);
        sink.write_little_endian<uint32_t, schema::index>(index);
        BC_ASSERT(sink.get_write_position() == schema::input::sk);
        return value;
    }

    struct slab
      : public schema::input
    {
        linkage<pk> count() const NOEXCEPT
        {
            return pk + sk +
                schema::tx +
                variable_size(index) +
                sizeof(uint32_t) +
                script.serialized_size(true) +
                witness.serialized_size(true);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<uint32_t, schema::tx>();
            index     = system::narrow_cast<uint32_t>(source.read_variable());
            sequence  = source.read_little_endian<uint32_t>();
            script    = system::chain::script(source, true);
            witness   = system::chain::witness(source, true);
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<uint32_t, schema::tx>(parent_fk);
            sink.write_variable(index);
            sink.write_little_endian<uint32_t>(sequence);
            script.to_data(sink, true);
            witness.to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return parent_fk == other.parent_fk
                && index == other.index
                && sequence == other.sequence
                && script == other.script
                && witness == other.witness;
        }

        uint32_t parent_fk{}; // parent fk *is* a required query.
        uint32_t index{};     // own (parent-relative) index not a required query.
        uint32_t sequence{};
        system::chain::script script{};
        system::chain::witness witness{};
    };

    struct slab_composite_sk
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_forward<sk>();
            return source;
        }

        search<sk> key{};
    };

    struct slab_decomposed_sk
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            // TODO: generalize/optimize.
            source.rewind_bytes(sk);
            point_fk = source.read_little_endian<uint32_t, schema::tx>();
            point_index = source.read_little_endian<uint32_t, schema::index>();
            return source;
        }

        uint32_t point_fk{};
        uint32_t point_index{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
