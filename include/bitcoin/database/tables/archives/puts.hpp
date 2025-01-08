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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_PUTS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_PUTS_HPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Puts is an blob of spend and output fk records.
struct puts
  : public no_map<schema::puts>
{
    using spend = linkage<schema::spend_>;
    using out = linkage<schema::put>;
    using spend_links = std_vector<spend::integer>;
    using output_links = std_vector<out::integer>;
    using no_map<schema::puts>::nomap;

    // TODO: There is a potential optimization available given that the inputs
    // (spend puts) for a given transaction are sequential. This means that an
    // offset with count are sufficient to store and number of spends. This is
    // more efficient if store averages more than two spends per tx (maybe).

    struct slab
      : public schema::puts
    {
        link count() const NOEXCEPT
        {
            const auto fks = spend_fks.size() * spend::size +
                out_fks.size() * out::size;
            return system::possible_narrow_cast<link::integer>(fks);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            std::for_each(spend_fks.begin(), spend_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<spend::integer, spend::size>();
            });

            std::for_each(out_fks.begin(), out_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<out::integer, out::size>();
            });

            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            std::for_each(spend_fks.begin(), spend_fks.end(), [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<spend::integer, spend::size>(fk);
            });

            std::for_each(out_fks.begin(), out_fks.end(), [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<out::integer, out::size>(fk);
            });

            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return spend_fks == other.spend_fks
                && out_fks == other.out_fks;
        }

        spend_links spend_fks{};
        output_links out_fks{};
    };

    struct get_spends
      : public schema::puts
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            std::for_each(spend_fks.begin(), spend_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<spend::integer, spend::size>();
            });

            return source;
        }

        spend_links spend_fks{};
    };

    struct get_outs
      : public schema::puts
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            std::for_each(out_fks.begin(), out_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<out::integer, out::size>();
            });

            return source;
        }

        output_links out_fks{};
    };

    struct get_spend_at
      : public schema::puts
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            spend_fk = source.read_little_endian<spend::integer, spend::size>();
            return source;
        }

        spend::integer spend_fk{};
    };

    struct get_output_at
      : public schema::puts
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            out_fk = source.read_little_endian<out::integer, out::size>();
            return source;
        }

        out::integer out_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
