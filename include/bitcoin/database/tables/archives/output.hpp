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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_OUTPUT_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_OUTPUT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

BC_PUSH_WARNING(NO_NEW_OR_DELETE)

/// Output is a blob (set of non-searchable slabs).
/// Output can be obtained by fk navigation (eg from tx/index). 
struct output
  : public array_map<schema::output>
{
    using tx = linkage<schema::tx>;
    using ix = linkage<schema::index>;
    using array_map<schema::output>::arraymap;

    struct slab
      : public schema::output
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                tx::size +
                variable_size(index) +
                variable_size(value) +
                script.serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            index     = narrow_cast<ix::integer>(source.read_variable());
            value     = source.read_variable();
            script    = chain::script(source, true);
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(writer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_variable(index);
            sink.write_variable(value);
            script.to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return parent_fk == other.parent_fk
                && index == other.index
                && value == other.value
                && script == other.script;
        }

        tx::integer parent_fk{};
        ix::integer index{};
        uint64_t value{};
        system::chain::script script{};
    };

    // Cannot use output{ sink } because database output.value is varint.
    struct only
      : public schema::output
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            source.skip_bytes(tx::size);
            source.skip_variable();
            output = to_shared(new chain::output
            {
                source.read_variable(),
                to_shared(new chain::script{ source, true })
            });
            return source;
        }

        system::chain::output::cptr output{};
    };

    struct get_value
      : public schema::output
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(tx::size);
            source.skip_variable();
            value = source.read_variable();
            return source;
        }

        uint64_t value{};
    };

    struct get_parent
      : public schema::output
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        tx::integer parent_fk{};
    };

    struct get_point
      : public schema::output
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            index     = narrow_cast<ix::integer>(source.read_variable());
            return source;
        }

        tx::integer parent_fk{};
        ix::integer index{};
    };

    struct slab_put_ref
      : public schema::output
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                tx::size +
                variable_size(index) +
                variable_size(output.value()) +
                output.script().serialized_size(true));
        }

        inline bool to_data(writer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_variable(index);
            sink.write_variable(output.value());
            output.script().to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        tx::integer parent_fk{};
        ix::integer index{};
        const system::chain::output& output{};
    };
};

BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
