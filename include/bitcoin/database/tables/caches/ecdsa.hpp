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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_ECDSA_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_ECDSA_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {
    
/// ecdsa is an array of ecdsa signature validation records.
struct ecdsa
  : public no_map<schema::ecdsa>
{
    using header = schema::header::link;
    using no_map<schema::ecdsa>::nomap;

    struct record
      : public schema::ecdsa
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            digest = source.read_hash();
            public_key = source.read_forward<system::ec_compressed_size>();
            signature = source.read_forward<system::ec_signature_size>();
            header_fk = source.read_little_endian<header::integer, header::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(digest);
            sink.write_bytes(public_key);
            sink.write_bytes(signature);
            sink.write_little_endian<header::integer, header::size>(header_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return digest == other.digest
                && public_key == other.public_key
                && signature == other.signature
                && header_fk == other.header_fk;
        }

        system::hash_digest digest{};
        system::ec_compressed public_key{};
        system::ec_signature signature{};
        header::integer header_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
