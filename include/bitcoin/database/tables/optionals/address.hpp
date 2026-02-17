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
#ifndef LIBBITCOIN_DATABASE_TABLES_OPTIONALS_ADDRESS_HPP
#define LIBBITCOIN_DATABASE_TABLES_OPTIONALS_ADDRESS_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// address is a record multimap of output fk records.
struct address
  : public hash_map<schema::address>
{
    using out = schema::output::link;
    using hash_map<schema::address>::hashmap;

    struct record
      : public schema::address
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            output_fk = source.read_little_endian<out::integer, out::size>();
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<out::integer, out::size>(output_fk);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return output_fk == other.output_fk;
        }

        out::integer output_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
