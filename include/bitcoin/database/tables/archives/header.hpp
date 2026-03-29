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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Header is a canonical record hash table.
struct header
  : public hash_map<schema::header>
{
    using head = schema::header::link;
    using hash_map<schema::header>::hashmap;
    static constexpr auto offset = head::bits;
    static_assert(offset < to_bits(head::size));

    static constexpr size_t skip_to_height =
        context::flag_t::size;

    static constexpr size_t skip_to_mtp =
        skip_to_height +
        context::height_t::size;

    static constexpr size_t skip_to_parent =
        skip_to_mtp +
        sizeof(uint32_t);

    static constexpr size_t skip_to_version =
        skip_to_parent +
        link::size;

    static constexpr size_t skip_to_timestamp =
        skip_to_version +
        sizeof(uint32_t);

    static constexpr size_t skip_to_bits =
        skip_to_timestamp +
        sizeof(uint32_t);

    static constexpr head::integer merge(bool milestone,
        head::integer parent_fk) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT_MSG(!get_right(parent_fk, offset), "overflow");
        return set_right(parent_fk, offset, milestone);
    }

    static constexpr bool is_milestone(link::integer merged) NOEXCEPT
    {
        return system::get_right(merged, offset);
    }

    static constexpr link::integer to_parent(link::integer merged) NOEXCEPT
    {
        return system::set_right(merged, offset, false);
    }

    struct record
      : public schema::header
    {        
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::from_data(source, ctx);
            const auto merged = source.read_little_endian<link::integer, link::size>();
            milestone   = is_milestone(merged);
            parent_fk   = to_parent(merged);
            version     = source.read_little_endian<uint32_t>();
            timestamp   = source.read_little_endian<uint32_t>();
            bits        = source.read_little_endian<uint32_t>();
            nonce       = source.read_little_endian<uint32_t>();
            merkle_root = source.read_hash();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_little_endian<link::integer, link::size>(merge(milestone, parent_fk));
            sink.write_little_endian<uint32_t>(version);
            sink.write_little_endian<uint32_t>(timestamp);
            sink.write_little_endian<uint32_t>(bits);
            sink.write_little_endian<uint32_t>(nonce);
            sink.write_bytes(merkle_root);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return ctx         == other.ctx
                && milestone   == other.milestone
                && parent_fk   == other.parent_fk
                && version     == other.version
                && timestamp   == other.timestamp
                && bits        == other.bits
                && nonce       == other.nonce
                && merkle_root == other.merkle_root;
        }

        context ctx{};
        bool milestone{};
        link::integer parent_fk{};
        uint32_t version{};
        uint32_t timestamp{};
        uint32_t bits{};
        uint32_t nonce{};
        hash_digest merkle_root{};
    };

    // This is redundant with record_put_ptr except this does not capture.
    struct put_ref
      : public schema::header
    {
        // header.previous_block_hash() ignored.
        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_little_endian<link::integer, link::size>(merge(milestone, parent_fk));
            sink.write_little_endian<uint32_t>(header.version());
            sink.write_little_endian<uint32_t>(header.timestamp());
            sink.write_little_endian<uint32_t>(header.bits());
            sink.write_little_endian<uint32_t>(header.nonce());
            sink.write_bytes(header.merkle_root());
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const context& ctx{};
        const bool milestone{};
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

        // null_hash is the required default.
        key key{};
    };

    // This is an optimization which is otherwise redundant with get_key().
    struct record_sk
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            return source;
        }

        // null_hash is the required default.
        key key{};
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

    struct get_flags
      : public schema::header
    {
        using flag_t = context::flag_t;
        inline bool from_data(reader& source) NOEXCEPT
        {
            flags = source.read_little_endian<flag_t::integer, flag_t::size>();
            return source;
        }

        flag_t::integer flags{};
    };

    struct get_height
      : public schema::header
    {
        using height_t = context::height_t;
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_height);
            height = source.read_little_endian<height_t::integer, height_t::size>();
            return source;
        }

        height_t::integer height{};
    };

    struct get_mtp
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_mtp);
            mtp = source.read_little_endian<uint32_t>();
            return source;
        }

        context::mtp_t mtp{};
    };

    struct get_parent_fk
      : public schema::header
    {        
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_parent);
            parent_fk = to_parent(source.read_little_endian<link::integer, link::size>());
            return source;
        }

        link::integer parent_fk{};
    };

    struct get_version
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_version);
            version = source.read_little_endian<uint32_t>();
            return source;
        }

        uint32_t version{};
    };

    struct get_timestamp
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_timestamp);
            timestamp = source.read_little_endian<uint32_t>();
            return source;
        }

        uint32_t timestamp{};
    };

    struct get_bits
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_bits);
            bits = source.read_little_endian<uint32_t>();
            return source;
        }

        uint32_t bits{};
    };

    struct get_milestone
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_parent);
            milestone = is_milestone(source.read_little_endian<link::integer, link::size>());
            return source;
        }

        bool milestone{};
    };

    struct get_check_context
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.rewind_bytes(sk);
            key = source.read_hash();
            context::from_data(source, ctx);
            source.skip_bytes(skip_to_timestamp - skip_to_parent);
            timestamp = source.read_little_endian<uint32_t>();
            return source;
        }

        key key{};
        context ctx{};
        uint32_t timestamp{};
    };

    struct wire_key
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            const auto version_size = sizeof(uint32_t);
            source.rewind_bytes(sk);
            flipper.skip_bytes(version_size);
            flipper.write_bytes(source.read_hash());
            return source;
        }

        system::byteflipper& flipper;
    };

    struct wire_header
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            const auto version_size = sizeof(uint32_t);
            const auto time_bits_nonce_size = 3u * sizeof(uint32_t);
            source.skip_bytes(skip_to_parent);
            parent_fk = to_parent(source.read_little_endian<link::integer, link::size>());
            flipper.write_bytes(source.read_bytes(version_size));
            flipper.write_bytes(system::null_hash);
            source.skip_bytes(time_bits_nonce_size);
            flipper.write_bytes(source.read_hash());
            source.rewind_bytes(time_bits_nonce_size + schema::hash);
            flipper.write_bytes(source.read_bytes(time_bits_nonce_size));
            return source;
        }

        system::byteflipper& flipper;
        link::integer parent_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
