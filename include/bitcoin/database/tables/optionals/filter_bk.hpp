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
#ifndef LIBBITCOIN_DATABASE_TABLES_OPTIONALS_FILTER_BK_HPP
#define LIBBITCOIN_DATABASE_TABLES_OPTIONALS_FILTER_BK_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// filter_bk is a slab arraymap of neutrino filter headers.
struct filter_bk
  : public array_map<schema::filter_bk>
{
    using array_map<schema::filter_bk>::arraymap;

    struct get_head
      : public schema::filter_bk
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            head = source.read_hash();
            return source;
        }

        hash_digest head{};
    };

    struct put_ref
      : public schema::filter_bk
    {
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_bytes(head);
            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const hash_digest& head{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
