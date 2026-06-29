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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_OUTS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_OUTS_HPP

#include <algorithm>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Outs is a record output fk records.
struct outs
  : public no_map<schema::outs>
{
    using out = schema::output::link;
    using tx = schema::transaction::link;
    using output_links = std::vector<out::integer>;
    using no_map<schema::outs>::nomap;

    struct record
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(out_fks.size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            std::ranges::for_each(out_fks, [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<out::integer, out::size>();
            });

            BC_ASSERT(!source || source.get_read_position() == count() * out::size);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            std::ranges::for_each(out_fks, [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<out::integer, out::size>(fk);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return out_fks == other.out_fks;
        }

        output_links out_fks{};
    };

    struct get_output
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            out_fk = source.read_little_endian<out::integer, out::size>();
            return source;
        }

        out::integer out_fk{};
    };

    struct put_ref
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tx_.outputs());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            static_assert(tx::size <= sizeof(uint64_t));
            constexpr auto value_parent = sizeof(uint64_t) - tx::size;

            auto out_fk = output_fk;
            const auto& outs = *tx_.outputs_ptr();
            std::ranges::for_each(outs, [&](const auto& out) NOEXCEPT
            {
                sink.write_little_endian<out::integer, out::size>(out_fk);

                // Calculate next corresponding output fk from serialized size.
                // (variable_size(value) + (value + script)) - (value - parent)
                out_fk += (variable_size(out->value()) +
                    out->serialized_size() - value_parent);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const out::integer output_fk{};
        const system::chain::transaction& tx_{};
    };

    struct put_view
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tx_.outputs());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            auto out_fk = output_fk;
            auto stream = tx_.get_outputs_stream();
            read::bytes::fast source{ stream };
            for (size_t out{}; out < tx_.outputs(); ++out)
            {
                sink.write_little_endian<out::integer, out::size>(out_fk);
                const auto value = source.read_8_bytes_little_endian();
                const auto bytes = source.read_size();
                source.skip_bytes(bytes);
                out_fk += tx::size + variable_size(value)
                    + variable_size(bytes) + bytes;
            }

            BC_ASSERT(source);
            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink && source;
        }

        const out::integer output_fk{};
        const system::chain::transaction_view& tx_;
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
