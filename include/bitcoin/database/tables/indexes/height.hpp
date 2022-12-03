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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_HEIGHT_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_HEIGHT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// height is an array of header fk records.
struct height
  : public array_map<schema::height>
{
    using block = linkage<schema::block>;
    using array_map<schema::height>::arraymap;

    struct record
      : public schema::height
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            header_fk = source.read_little_endian<block::integer, block::size>();
            return source;
        }

        inline bool to_data(writer& sink) const NOEXCEPT
        {
            sink.write_little_endian<block::integer, block::size>(header_fk);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return header_fk == other.header_fk;
        }

        block::integer header_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
