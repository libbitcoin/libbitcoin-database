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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_BK_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_BK_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// validated_bk is a slab hashmap of block validation state.
struct validated_bk
  : public hash_map<schema::validated_bk>
{
    using coding = linkage<schema::code>;
    using hash_map<schema::validated_bk>::hashmap;

    struct slab
      : public schema::validated_bk
    {
        link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(pk + sk +
                coding::size +
                variable_size(fees));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            code = source.read_little_endian<coding::integer, coding::size>();
            fees = source.read_variable();
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<coding::integer, coding::size>(code);
            sink.write_variable(fees);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return code == other.code
                && fees == other.fees;
        }

        coding::integer code{};
        uint64_t fees{};
    };

    struct slab_get_code
      : public schema::validated_bk
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            code = source.read_little_endian<coding::integer, coding::size>();
            return source;
        }
    
        coding::integer code{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
