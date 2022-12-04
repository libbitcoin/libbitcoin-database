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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_BOOTSTRAP_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_BOOTSTRAP_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// bootstrap is an array of header hash records (initial blockchain).
struct bootstrap
  : public array_map<schema::bootstrap>
{
    using array_map<schema::bootstrap>::arraymap;

    struct record
      : public schema::bootstrap
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            block_hash = source.read_hash();
            return source;
        }

        inline bool to_data(writer& sink) const NOEXCEPT
        {
            sink.write_bytes(block_hash);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return block_hash == other.block_hash;
        }

        hash_digest block_hash{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
