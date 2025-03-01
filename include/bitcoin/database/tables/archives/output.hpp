/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

/// Output is a blob (set of non-searchable slabs).
/// Output can be obtained by fk navigation (eg from tx/index). 
struct output
  : public no_map<schema::output>
{
    using tx = linkage<schema::tx>;
    using no_map<schema::output>::nomap;

    struct slab
      : public schema::output
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                tx::size +
                variable_size(value) +
                script.serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            value     = source.read_variable();
            script = chain::script{ source, true };
            BC_ASSERT(!source || source.get_read_position() == count());
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            sink.write_variable(value);
            script.to_data(sink, true);
            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return parent_fk == other.parent_fk
                && value == other.value
                && script == other.script;
        }

        tx::integer parent_fk{};
        uint64_t value{};
        system::chain::script script{};
    };

    // Cannot use output{ sink } because database output.value is varint.
    struct only
      : public schema::output
    {
        link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            source.skip_bytes(tx::size);
            output = to_shared(new chain::output
            {
                source.read_variable(),
                to_shared<chain::script>(source, true)
            });

            return source;
        }

        system::chain::output::cptr output{};
    };

    struct get_parent
      : public schema::output
    {
        link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        tx::integer parent_fk{};
    };

    struct get_value
      : public schema::output
    {
        link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(tx::size);
            value = source.read_variable();
            return source;
        }

        uint64_t value{};
    };

    struct put_ref
      : public schema::output
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            static_assert(tx::size <= sizeof(uint64_t));
            static constexpr auto value_parent_difference = sizeof(uint64_t) -
                tx::size;

            const auto& outs = *tx_.outputs_ptr();
            const auto other = outs.size() * value_parent_difference;
            const auto outputs = std::accumulate(outs.begin(), outs.end(), zero,
                [](size_t total, const auto& out) NOEXCEPT
                {
                    // size cached, so this is free, includes sizeof(value).
                    return total + variable_size(out->value()) +
                        out->serialized_size();
                });

            // Converts value from fixed size wire encoding to variable.
            // (variable_size(value) + (value + script)) - (value - parent)
            return possible_narrow_cast<link::integer>(outputs - other);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto& outs = *tx_.outputs_ptr();
            std::for_each(outs.begin(), outs.end(), [&](const auto& out) NOEXCEPT
            {
                sink.write_little_endian<tx::integer, tx::size>(parent_fk);
                sink.write_variable(out->value());
                out->script().to_data(sink, true);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        const tx::integer parent_fk{};
        const system::chain::transaction& tx_{};
    };
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
