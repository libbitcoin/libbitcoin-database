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
struct header
  : public hash_map<schema::header>
{
    using search_key = search<schema::hash>;
    using hash_map<schema::header>::hashmap;

    struct record
      : public schema::header
    {        
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::from_data(source, ctx);
            parent_fk   = source.read_little_endian<link::integer, link::size>();
            version     = source.read_little_endian<uint32_t>();
            timestamp   = source.read_little_endian<uint32_t>();
            bits        = source.read_little_endian<uint32_t>();
            nonce       = source.read_little_endian<uint32_t>();
            merkle_root = source.read_hash();
            BC_ASSERT(source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_little_endian<link::integer, link::size>(parent_fk);
            sink.write_little_endian<uint32_t>(version);
            sink.write_little_endian<uint32_t>(timestamp);
            sink.write_little_endian<uint32_t>(bits);
            sink.write_little_endian<uint32_t>(nonce);
            sink.write_bytes(merkle_root);
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return ctx       == other.ctx
                && parent_fk   == other.parent_fk
                && version     == other.version
                && timestamp   == other.timestamp
                && bits        == other.bits
                && nonce       == other.nonce
                && merkle_root == other.merkle_root;
        }

        context ctx{};
        link::integer parent_fk{};
        uint32_t version{};
        uint32_t timestamp{};
        uint32_t bits{};
        uint32_t nonce{};
        hash_digest merkle_root{};
    };

    struct record_put_ptr
      : public schema::header
    {
        // header_ptr->previous_block_hash() ignored.
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            BC_ASSERT(header_ptr);
            context::to_data(sink, ctx);
            sink.write_little_endian<link::integer, link::size>(parent_fk);
            sink.write_little_endian<uint32_t>(header_ptr->version());
            sink.write_little_endian<uint32_t>(header_ptr->timestamp());
            sink.write_little_endian<uint32_t>(header_ptr->bits());
            sink.write_little_endian<uint32_t>(header_ptr->nonce());
            sink.write_bytes(header_ptr->merkle_root());
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        const context ctx{};
        const link::integer parent_fk{};
        system::chain::header::cptr header_ptr{};
    };

    // This is redundant with record_put_ptr except this does not capture.
    struct record_put_ref
      : public schema::header
    {
        // header.previous_block_hash() ignored.
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_little_endian<link::integer, link::size>(parent_fk);
            sink.write_little_endian<uint32_t>(header.version());
            sink.write_little_endian<uint32_t>(header.timestamp());
            sink.write_little_endian<uint32_t>(header.bits());
            sink.write_little_endian<uint32_t>(header.nonce());
            sink.write_bytes(header.merkle_root());
            BC_ASSERT(sink.get_write_position() == minrow);
            return sink;
        }

        const context& ctx{};
        const link::integer parent_fk{};
        const system::chain::header& header;
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

        search_key key{};
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

        search_key key{};
    };

    struct record_flags
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            using state = context::state;
            flags = source.read_little_endian<state::integer, state::size>();
            return source;
        }

        context::state::integer flags{};
    };

    struct record_height
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            using block = context::block;
            source.skip_bytes(context::state::size);
            height = source.read_little_endian<block::integer, block::size>();
            return source;
        }

        context::block::integer height{};
    };

    struct record_mtp
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(context::state::size + context::block::size);
            mtp = source.read_little_endian<uint32_t>();
            return source;
        }

        uint32_t mtp{};
    };

    struct record_context
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::from_data(source, ctx);
            return source;
        }

        context ctx{};
    };
};

BC_POP_WARNING()

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
