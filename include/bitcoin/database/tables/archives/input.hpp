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

BC_PUSH_WARNING(NO_NEW_OR_DELETE)

/// Input is searchable by point_fk/index (fP) of the output that it spends.
/// This makes input a multimap, as multiple inputs can spend a given output.
struct input
  : public hash_map<schema::input>
{
    using tx = linkage<schema::tx>;
    using ix = linkage<schema::index>;
    using search_key = search<schema::tx_fp>;
    using hash_map<schema::input>::hashmap;

    /// Generate composite key.
    /// Foreign point index limited to 3 bytes, which cannot hold null_index.
    /// Sentinel 0xffffffff truncated to 0x00ffffff upon write and explicitly
    /// restored to 0xffffffff upon read.
    static const search_key to_point(tx::integer fk, ix::integer index) NOEXCEPT
    {
        // TODO: optimize (mask each byte and or together).
        search_key value{};
        system::write::bytes::copy sink(value);
        sink.write_little_endian<tx::integer, tx::size>(fk);
        sink.write_little_endian<ix::integer, ix::size>(index);
        BC_ASSERT(sink.get_write_position() == array_count<search_key>);
        return value;
    }

    struct slab
      : public schema::input
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                tx::size +
                variable_size(index) +
                sizeof(uint32_t) +
                script.serialized_size(true) +
                witness.serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            index     = system::narrow_cast<ix::integer>(source.read_variable());
            sequence  = source.read_little_endian<uint32_t>();
            script    = system::chain::script(source, true);
            witness   = system::chain::witness(source, true);
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
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

        tx::integer parent_fk{};
        ix::integer index{};
        uint32_t sequence{};
        system::chain::script script{};
        system::chain::witness witness{};
    };

    // Cannot return complete input because input.point is not available.
    struct only
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            source.skip_bytes(tx::size);
            source.skip_variable();
            sequence = source.read_little_endian<uint32_t>();
            script = to_shared(new chain::script{ source, true });
            witness = to_shared(new chain::witness{ source, true });
            return source;
        }

        uint32_t sequence{};
        system::chain::script::cptr script{};
        system::chain::witness::cptr witness{};
    };

    ////struct slab_put_ptr
    ////  : public schema::input
    ////{
    ////    link count() const NOEXCEPT
    ////    {
    ////        return system::possible_narrow_cast<link::integer>(pk + sk +
    ////            tx::size +
    ////            variable_size(index) +
    ////            sizeof(uint32_t) +
    ////            input->script().serialized_size(true) +
    ////            input->witness().serialized_size(true));
    ////    }
    ////
    ////    inline bool to_data(finalizer& sink) const NOEXCEPT
    ////    {
    ////        sink.write_little_endian<tx::integer, tx::size>(parent_fk);
    ////        sink.write_variable(index);
    ////        sink.write_little_endian<uint32_t>(input->sequence());
    ////        input->script().to_data(sink, true);
    ////        input->witness().to_data(sink, true);
    ////        BC_ASSERT(sink.get_write_position() == count());
    ////        return sink;
    ////    }
    ////
    ////    tx::integer parent_fk{};
    ////    ix::integer index{};
    ////    const system::chain::input::cptr input{};
    ////};

    struct slab_put_ref
      : public schema::input
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                tx::size +
                variable_size(index) +
                sizeof(uint32_t) +
                input.script().serialized_size(true) +
                input.witness().serialized_size(true));
        }

        // Cannot use input.to_date(sink) because it includes input.point.
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_variable(index);
            sink.write_little_endian<uint32_t>(input.sequence());
            input.script().to_data(sink, true);
            input.witness().to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        tx::integer parent_fk{};
        ix::integer index{};
        const system::chain::input& input{};
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

        search_key key{};
    };

    struct slab_decomposed_sk
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            point_fk    = source.read_little_endian<tx::integer, tx::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            // Restore truncated null_index sentinel.
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            return source;
        }

        tx::integer point_fk{};
        ix::integer point_index{};
    };

    struct only_with_decomposed_sk
      : public only
    {
        BC_PUSH_WARNING(NO_METHOD_HIDING)
        inline bool from_data(reader& source) NOEXCEPT
        BC_POP_WARNING()
        {
            source.rewind_bytes(sk);
            point_fk    = source.read_little_endian<tx::integer, tx::size>();
            point_index = source.read_little_endian<ix::integer, ix::size>();

            // Restore truncated null_index sentinel.
            if (point_index == ix::terminal)
                point_index = system::chain::point::null_index;

            return only::from_data(source);
        }

        tx::integer point_fk{};
        ix::integer point_index{};
    };

    ////struct slab_with_decomposed_sk
    ////  : public slab
    ////{
    ////    BC_PUSH_WARNING(NO_METHOD_HIDING)
    ////    inline bool from_data(reader& source) NOEXCEPT
    ////    BC_POP_WARNING()
    ////    {
    ////        source.rewind_bytes(sk);
    ////        point_fk    = source.read_little_endian<tx::integer, tx::size>();
    ////        point_index = source.read_little_endian<ix::integer, ix::size>();
    ////
    ////        // Restore truncated null_index sentinel.
    ////        if (point_index == ix::terminal)
    ////            point_index = system::chain::point::null_index;
    ////
    ////        return slab::from_data(source);
    ////    }
    ////
    ////    tx::integer point_fk{};
    ////    ix::integer point_index{};
    ////};
};

BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
