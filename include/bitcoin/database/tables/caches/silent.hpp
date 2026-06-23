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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_SILENT_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_SILENT_HPP

#include <algorithm>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// silent is an array of silent payment scan records.
struct silent
  : public no_map<schema::silent>
{
    using tx = schema::transaction::link;
    using no_map<schema::silent>::nomap;

    struct record
      : public schema::silent
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            prefix = source.read_little_endian<uint64_t>();
            compressed = source.read_forward<system::ec_compressed_size>();
            tx_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<uint64_t>(prefix);
            sink.write_bytes(compressed);
            sink.write_little_endian<tx::integer, tx::size>(tx_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return prefix == other.prefix
                && compressed == other.compressed
                && tx_fk == other.tx_fk;
        }

        uint64_t prefix{};
        system::ec_compressed compressed{};
        tx::integer tx_fk{};
    };

    struct records
      : public schema::silent
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(prefixes.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            // The prefix must be read from ec_xonly[0..7] as LE.
            // Disk sequence will be [0..7] (with no byteswap on LE hardware).
            std::for_each(prefixes.cbegin(), prefixes.cend(),
                [&](uint64_t prefix) NOEXCEPT
                {
                    sink.write_little_endian<uint64_t>(prefix);
                    sink.write_bytes(compressed);
                    sink.write_little_endian<tx::integer, tx::size>(tx_fk);
                });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        std::vector<uint64_t> prefixes{};
        system::ec_compressed compressed{};
        tx::integer tx_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
