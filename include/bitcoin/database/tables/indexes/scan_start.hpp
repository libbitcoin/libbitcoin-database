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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_SCAN_START_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_SCAN_START_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// scan_start is a head-only array of scan fks, each starting the scan range
/// of a header, indexed by header.fk less the first taproot header.fk.
struct scan_start
  : public head_map<schema::scan_start>
{
    using head_map<schema::scan_start>::headmap;

    /// Replace the scan fk.
    struct record
      : public schema::scan_start
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(link::size);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<link::integer, link::size>(start);
            return sink;
        }

        link::integer start{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
