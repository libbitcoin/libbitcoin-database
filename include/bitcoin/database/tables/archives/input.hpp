/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

#include <algorithm>
#include <memory>
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
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            script = std::make_shared<const chain::script>(source, true);
            witness = witnessed ?
                std::make_shared<const chain::witness>(source, true) :
                std::make_shared<const chain::witness>();
            return source;
        }

        bool witnessed{};
        system::chain::script::cptr script{};
        system::chain::witness::cptr witness{};
    };

    struct get_script
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            script = std::make_shared<const chain::script>(source, true);
            return source;
        }

        system::chain::script::cptr script{};
    };

    struct get_witness
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            source.skip_bytes(source.read_size());
            witness = std::make_shared<const chain::witness>(source, true);
            return source;
        }

        system::chain::witness::cptr witness{};
    };

    struct put_ref
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            constexpr auto sequence_point_size = sizeof(uint32_t) +
                chain::point::serialized_size();

            const auto& ins = *tx_.inputs_ptr();
            const auto other = ins.size() * sequence_point_size;
            const auto inputs = std::accumulate(ins.cbegin(), ins.cend(), zero,
                [](size_t total, const auto& in) NOEXCEPT
                {
                    // Includes zero stack size write for non-segregated inputs.
                    return total + in->serialized_size(true);
                });

            // (script + witness + sequence + point) - (sequence + point)
            return possible_narrow_cast<link::integer>(inputs - other);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto& ins = *tx_.inputs_ptr();
            std::ranges::for_each(ins, [&](const auto& in) NOEXCEPT
            {
                in->script().to_data(sink, true);
                in->witness().to_data(sink, true);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        const system::chain::transaction& tx_{};
    };

    struct put_view
      : public schema::input
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(tx_.input_table_size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            auto istream = tx_.get_inputs_stream();
            read::bytes::fast isource{ istream };

            if (tx_.is_segregated())
            {
                auto wstream = tx_.get_witnesses_stream();
                read::bytes::fast wsource{ wstream };

                for (size_t in{}; in < tx_.inputs(); ++in)
                {
                    // input script + witness stack
                    tx_.write_input_script(sink, isource);
                    isource.skip_bytes(sizeof(uint32_t));
                    tx_.write_witness(sink, wsource);
                }

                if (!wsource)
                    isource.invalidate();
            }
            else
            {
                for (size_t in{}; in < tx_.inputs(); ++in)
                {
                    // input script + empty witness stack
                    tx_.write_input_script(sink, isource);
                    isource.skip_bytes(sizeof(uint32_t));
                    sink.write_variable(zero);
                }
            }

            BC_ASSERT(isource);
            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink && isource;
        }

        const system::chain::transaction_view& tx_;
    };

    struct wire_script
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            // script (prefixed)
            const auto length = source.read_size();
            sink.write_variable(length);
            sink.write_bytes(source.read_bytes(length));
            return source;
        }

        bytewriter& sink;
    };

    struct wire_witness
      : public schema::input
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            // script (skip)
            source.skip_bytes(source.read_size());

            // witness (count)
            const auto count = source.read_size();
            sink.write_variable(count);

            // witness (prefixed)
            for (size_t element{}; element < count; ++element)
            {
                const auto length = source.read_size();
                sink.write_variable(length);
                sink.write_bytes(source.read_bytes(length));
            }

            return source;
        }

        bytewriter& sink;
    };
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
