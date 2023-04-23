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
#include <bitcoin/database/tables/indexes/spend.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {
    
BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

/// Input is a blob (set of non-searchable slabs).
/// Input can be obtained by fk navigation (eg from tx/index). 
struct input
  : public array_map<schema::input>
{
    using ix = linkage<schema::index>;
    using pt = linkage<schema::point::pk>;
    using tx = linkage<schema::transaction::pk>;
    using foreign_point = table::spend::search_key;
    using array_map<schema::input>::arraymap;

    struct slab
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                pt::size +
                (null_pt(point_fk) ? zero : variable_size(point_index)) +
                tx::size +
                sizeof(uint32_t) +
                script.serialized_size(true) +
                witness.serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            read_point(source, *this);
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            sequence = source.read_little_endian<uint32_t>();
            script = system::chain::script(source, true);
            witness = system::chain::witness(source, true);
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(writer& sink) const NOEXCEPT
        {
            write_point(sink, *this);
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_little_endian<uint32_t>(sequence);
            script.to_data(sink, true);
            witness.to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return point_fk == other.point_fk
                && point_index == other.point_index
                && parent_fk == other.parent_fk
                && sequence == other.sequence
                && script == other.script
                && witness == other.witness;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_pt(point_fk);
        }

        inline foreign_point prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        tx::integer parent_fk{};
        uint32_t sequence{};
        system::chain::script script{};
        system::chain::witness witness{};
    };

    struct only
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            skip_point(source);
            source.skip_bytes(tx::size);
            sequence = source.read_little_endian<uint32_t>();
            script = system::to_shared<system::chain::script>(source, true);
            witness = system::to_shared<system::chain::witness>(source, true);
            return source;
        }

        uint32_t sequence{};
        system::chain::script::cptr script{};
        system::chain::witness::cptr witness{};
    };

    struct only_with_prevout
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                pt::size +
                (null_pt(point_fk) ? zero : variable_size(point_index)) +
                tx::size +
                sizeof(uint32_t) +
                script->serialized_size(true) +
                witness->serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            read_point(source, *this);
            source.skip_bytes(tx::size);
            sequence = source.read_little_endian<uint32_t>();
            script = system::to_shared<system::chain::script>(source, true);
            witness = system::to_shared<system::chain::witness>(source, true);
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_pt(point_fk);
        }

        inline foreign_point prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        ////tx::integer parent_fk{};
        uint32_t sequence{};
        system::chain::script::cptr script{};
        system::chain::witness::cptr witness{};
    };

    struct only_from_prevout
      : public schema::input
    {
        // Returns complete input given input.point is provided.
        inline bool from_data(reader& source) NOEXCEPT
        {
            BC_ASSERT(prevout);
            skip_point(source);
            source.skip_bytes(tx::size);
            const auto sequence = source.read_little_endian<uint32_t>();
            input = system::to_shared(new system::chain::input
            {
                prevout,
                system::to_shared<system::chain::script>(source, true),
                system::to_shared<system::chain::witness>(source, true),
                sequence
            });

            return source;
        }

        const system::chain::point::cptr prevout{};
        system::chain::input::cptr input{};
    };

    struct get_prevout
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            read_point(source, *this);
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_pt(point_fk);
        }

        inline foreign_point prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
    };

    struct get_parent
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            skip_point(source);
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        tx::integer parent_fk{};
    };

    struct get_prevout_parent
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            read_point(source, *this);
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }
    
        inline bool is_null() const NOEXCEPT
        {
            return null_pt(point_fk);
        }

        inline foreign_point prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }
   
        pt::integer point_fk{};
        ix::integer point_index{};
        tx::integer parent_fk{};
    };

    struct get_prevout_parent_sequence
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            read_point(source, *this);
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            sequence = source.read_little_endian<uint32_t>();
            return source;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_pt(point_fk);
        }

        inline foreign_point prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        tx::integer parent_fk{};
        uint32_t sequence{};
    };

    struct put_ref
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                pt::size +
                (null_pt(point_fk) ? zero : variable_size(point_index)) +
                tx::size +
                sizeof(uint32_t) +
                input.script().serialized_size(true) +
                input.witness().serialized_size(true));
        }

        inline bool to_data(writer& sink) const NOEXCEPT
        {
            write_point(sink, *this);
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_little_endian<uint32_t>(input.sequence());
            input.script().to_data(sink, true);
            input.witness().to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool is_null() const NOEXCEPT
        {
            return null_pt(point_fk);
        }

        inline foreign_point prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        pt::integer point_fk{};
        ix::integer point_index{};
        tx::integer parent_fk{};
        const system::chain::input& input{};
    };

private:
    static constexpr bool null_pt(pt::integer fk) NOEXCEPT
    {
        return fk == pt::terminal;
    }

    static inline void skip_point(reader& source) NOEXCEPT
    {
        if (!null_pt(source.read_little_endian<pt::integer, pt::size>()))
            source.skip_variable();
    }

    static inline void write_point(writer& sink, auto& self) NOEXCEPT
    {
        sink.write_little_endian<pt::integer, pt::size>(self.point_fk);
        if (!null_pt(self.point_fk))
            sink.write_variable(self.point_index);
    }

    static inline void read_point(reader& source, auto& self) NOEXCEPT
    {
        self.point_fk = source.read_little_endian<pt::integer, pt::size>();
        self.point_index = null_pt(self.point_fk) ?
            system::chain::point::null_index :
            system::narrow_cast<ix::integer>(source.read_variable());
    }
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
