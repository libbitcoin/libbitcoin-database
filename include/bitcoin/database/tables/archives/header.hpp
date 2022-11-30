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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

BC_PUSH_WARNING(NO_NEW_OR_DELETE)

/// Header is a cononical record hash table.
class header
  : public hash_map<schema::header>
{
public:
    using hash_map<schema::header>::hashmap;

    struct record
      : public schema::header
    {        
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::read(source, context);
            parent_fk   = source.read_little_endian<uint32_t, schema::block>();
            version     = source.read_little_endian<uint32_t>();
            merkle_root = source.read_hash();
            timestamp   = source.read_little_endian<uint32_t>();
            bits        = source.read_little_endian<uint32_t>();
            nonce       = source.read_little_endian<uint32_t>();
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::write(sink, context);
            sink.write_little_endian<uint32_t, schema::block>(parent_fk);
            sink.write_little_endian<uint32_t>(version);
            sink.write_bytes(merkle_root);
            sink.write_little_endian<uint32_t>(timestamp);
            sink.write_little_endian<uint32_t>(bits);
            sink.write_little_endian<uint32_t>(nonce);
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return context     == other.context
                && parent_fk   == other.parent_fk
                && version     == other.version
                && merkle_root == other.merkle_root
                && timestamp   == other.timestamp
                && bits        == other.bits
                && nonce       == other.nonce;
        }

        context context{};
        uint32_t parent_fk{};
        uint32_t version{};
        hash_digest merkle_root{};
        uint32_t timestamp{};
        uint32_t bits{};
        uint32_t nonce{};
    };

    struct record_get_ptr
      : public schema::header
    {
        // header_ptr->merkle_root() ignored.
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::read(source, context);
            parent_fk = source.read_little_endian<uint32_t, schema::block>();
            header_ptr = system::to_shared(new system::chain::header
            {
                source.read_little_endian<uint32_t>(), // version
                hash_digest{},                         // parent (unused)
                source.read_hash(),                    // merkle_root
                source.read_little_endian<uint32_t>(), // timestamp
                source.read_little_endian<uint32_t>(), // bits
                source.read_little_endian<uint32_t>()  // nonce
            });
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        context context{};
        uint32_t parent_fk{};
        system::chain::header::cptr header_ptr{};
    };

    struct record_put_ptr
      : public schema::header
    {
        // header_ptr->merkle_root() ignored.
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            BC_ASSERT(header_ptr);
            context::write(sink, context);
            sink.write_little_endian<uint32_t, schema::block>(parent_fk);
            sink.write_little_endian<uint32_t>(header_ptr->version());
            sink.write_bytes(header_ptr->merkle_root());
            sink.write_little_endian<uint32_t>(header_ptr->timestamp());
            sink.write_little_endian<uint32_t>(header_ptr->bits());
            sink.write_little_endian<uint32_t>(header_ptr->nonce());
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        const context context{};
        const uint32_t parent_fk{};
        system::chain::header::cptr header_ptr{};
    };

    struct record_with_sk
      : public record
    {
        BC_PUSH_WARNING(NO_METHOD_HIDING)
        inline bool from_data(reader& source) NOEXCEPT
        BC_POP_WARNING()
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            return record::from_data(source);
        }

        hash_digest key{};
    };

    struct record_sk
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            return source;
        }

        hash_digest key{};
    };

    struct record_height
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            height = source.read_little_endian<uint32_t, schema::block>();
            return source;
        }

        uint32_t height{};
    };
};

BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
