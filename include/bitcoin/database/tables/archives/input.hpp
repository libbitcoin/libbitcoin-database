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
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

/// Input is a blob (set of non-searchable slabs).
/// Input can be obtained by fk navigation (eg from tx/puts/spend). 
struct input
  : public no_map<schema::input>
{
    using no_map<schema::input>::nomap;

    struct slab
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                script.serialized_size(true) +
                witness.serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            script = system::chain::script{ source, true };
            witness = system::chain::witness{ source, true };
            BC_ASSERT(!source || source.get_read_position() == count());
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            script.to_data(sink, true);
            witness.to_data(sink, true);
            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return script == other.script
                && witness == other.witness;
        }

        system::chain::script script{};
        system::chain::witness witness{};
    };

    struct get_ptrs
      : public schema::input
    {
        link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            script = to_shared(new chain::script{ source, true });
            witness = to_shared(new chain::witness{ source, true });
            return source;
        }

        system::chain::script::cptr script{};
        system::chain::witness::cptr witness{};
    };

    struct put_ref
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            static constexpr auto sequence_point_size = sizeof(uint32_t) +
                chain::point::serialized_size();

            const auto& ins = *tx_.inputs_ptr();
            const auto other = ins.size() * sequence_point_size;
            const auto inputs = std::accumulate(ins.begin(), ins.end(), zero,
                [](size_t total, const auto& in) NOEXCEPT
                {
                    // sizes cached, so this is free.
                    return total + in->serialized_size(true);
                });

            // (script + witness + sequence + point) - (sequence + point)
            return possible_narrow_cast<link::integer>(inputs - other);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto& ins = *tx_.inputs_ptr();
            std::for_each(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
            {
                in->script().to_data(sink, true);
                in->witness().to_data(sink, true);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        const system::chain::transaction& tx_{};
    };
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
