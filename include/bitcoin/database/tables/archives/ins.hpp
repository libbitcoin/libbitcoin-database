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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_INS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_INS_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Sequence is part of an input, denormalized here for confirmation.
struct ins
  : public no_map<schema::ins>
{
    using ix = linkage<schema::index>;
    using in = schema::input::link;
    using tx = schema::transaction::link;
    using no_map<schema::ins>::nomap;

    struct record
      : public schema::ins
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<uint32_t>(sequence);
            sink.write_little_endian<in::integer, in::size>(input_fk);
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return sequence == other.sequence
                && input_fk == other.input_fk
                && parent_fk == other.parent_fk;
        }

        uint32_t sequence{};
        in::integer input_fk{};
        tx::integer parent_fk{};
    };

    struct get_parent
      : public schema::ins
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(ix::size + sizeof(uint32_t) + in::size);
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            return source;
        }

        tx::integer parent_fk{};
    };

    struct get_input
      : public schema::ins
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            sequence = source.read_little_endian<uint32_t>();
            input_fk = source.read_little_endian<in::integer, in::size>();
            return source;
        }

        uint32_t sequence{};
        in::integer input_fk{};
    };

    struct put_ref
      : public schema::ins
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tx_.inputs());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            constexpr auto sequence_point_size = sizeof(uint32_t) +
                chain::point::serialized_size();

            auto in_fk = input_fk;
            const auto& ins = *tx_.inputs_ptr();
            std::for_each(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
            {
                sink.write_little_endian<uint32_t>(in->sequence());
                sink.write_little_endian<in::integer, in::size>(in_fk);
                sink.write_little_endian<tx::integer, tx::size>(parent_fk);

                // Calculate next corresponding input fk from serialized size.
                // (script + witness + sequence + point) - (sequence + point)
                in_fk += (in->serialized_size(true) - sequence_point_size);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const in::integer input_fk{};
        const tx::integer parent_fk{};
        const system::chain::transaction& tx_{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
