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
    using hash_map<schema::header>::hashmap;
    static constexpr size_t milestone_bit = 0;
    static constexpr size_t compact_bit = 1;

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

    static constexpr size_t skip_to_work =
        skip_to_bits +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        schema::hash;

    static constexpr size_t skip_to_flags =
        skip_to_work +
        schema::work;

    static constexpr uint8_t to_flags(bool milestone, bool compact) NOEXCEPT
    {
        using namespace system;
        return set_right(set_right(uint8_t{}, milestone_bit, milestone),
            compact_bit, compact);
    }

    static constexpr bool is_milestone(uint8_t flags) NOEXCEPT
    {
        return system::get_right(flags, milestone_bit);
    }

    static constexpr bool is_compact(uint8_t flags) NOEXCEPT
    {
        return system::get_right(flags, compact_bit);
    }

    /// Cumulative work is stored in the low order schema::work bytes.
    static inline bool is_storable(const uint256_t& work) NOEXCEPT
    {
        return is_zero(work >> to_bits(schema::work));
    }

    static inline uint256_t read_work(reader& source) NOEXCEPT
    {
        hash_digest work{};
        source.read_bytes(work.data(), schema::work);
        return system::to_uintx(work);
    }

    static inline void write_work(finalizer& sink,
        const uint256_t& work) NOEXCEPT
    {
        BC_ASSERT(is_storable(work));
        sink.write_bytes(system::from_uintx(work).data(), schema::work);
    }

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
            work        = read_work(source);
            const auto flags = source.read_byte();
            milestone   = is_milestone(flags);
            compact     = is_compact(flags);
            BC_ASSERT(!source || source.get_read_position() == minrow);
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
            write_work(sink, work);
            sink.write_byte(to_flags(milestone, compact));
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        context ctx{};
        bool milestone{};
        bool compact{};
        link::integer parent_fk{};
        uint32_t version{};
        uint32_t timestamp{};
        uint32_t bits{};
        uint32_t nonce{};
        hash_digest merkle_root{};
        uint256_t work{};
    };

    // This is redundant with record_put_ptr except this does not capture.
    struct put_ref
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
            write_work(sink, work);
            sink.write_byte(to_flags(milestone, compact));
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const context& ctx{};
        const bool milestone{};
        const bool compact{};
        const link::integer parent_fk{};
        const system::chain::header& header;
        const uint256_t& work;
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

    struct record_context_timestamp
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::from_data(source, ctx);
            source.skip_bytes(link::size + sizeof(uint32_t));
            timestamp = source.read_little_endian<uint32_t>();
            return source;
        }

        context ctx{};
        uint32_t timestamp{};
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
            parent_fk = source.read_little_endian<link::integer, link::size>();
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

    struct get_work
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_work);
            work = read_work(source);
            return source;
        }

        uint256_t work{};
    };

    struct get_milestone
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_flags);
            milestone = is_milestone(source.read_byte());
            return source;
        }

        bool milestone{};
    };

    struct get_compact
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(skip_to_flags);
            compact = is_compact(source.read_byte());
            return source;
        }

        bool compact{};
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

    struct wire_header
      : public schema::header
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            const auto time_bits_nonce_size = 3u * sizeof(uint32_t);
            const auto version_size = sizeof(uint32_t);
            source.skip_bytes(skip_to_version);
            sink.write_bytes(source.read_bytes(version_size));
            sink.write_bytes(parent_hash);
            source.skip_bytes(time_bits_nonce_size);
            sink.write_bytes(source.read_hash());
            source.rewind_bytes(time_bits_nonce_size + schema::hash);
            sink.write_bytes(source.read_bytes(time_bits_nonce_size));
            return source;
        }

        bytewriter& sink;
        hash_digest parent_hash{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
