/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_SPENDS_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_SPENDS_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// spends is an array of parent tx fk records, merged with coinbase (as
/// prevout), contiguous by spending tx and in input order.
struct spends
  : public no_map<schema::spends>
{
    using tx = schema::transaction::link;
    using no_map<schema::spends>::nomap;

    struct record
      : public schema::spends
    {
        static constexpr link count() NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            parent_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(parent_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        tx::integer parent_fk{};
    };

    struct get_refs
      : public schema::spends
    {
        using parents = std::vector<tx::integer>;

        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(parent_fks.size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            std::ranges::for_each(parent_fks, [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<tx::integer, tx::size>();
            });

            BC_ASSERT(!source || source.get_read_position() == count() * minrow);
            return source;
        }

        parents& parent_fks;
    };

    struct put_refs
      : public schema::spends
    {
        using parents = std::vector<tx::integer>;

        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(parent_fks.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            std::ranges::for_each(parent_fks, [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<tx::integer, tx::size>(fk);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const parents& parent_fks;
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
