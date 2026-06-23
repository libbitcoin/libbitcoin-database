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
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            digest = source.read_hash();
            point = source.read_forward<system::ec_compressed_size>();
            signature = source.read_forward<system::ec_signature_size>();
            pair = source.read_byte();
            group = source.read_little_endian<uint16_t>();
            header_fk = source.read_little_endian<header::integer, header::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(digest);
            sink.write_bytes(point);
            sink.write_bytes(signature);
            sink.write_byte(pair);
            sink.write_little_endian<uint16_t>(group);
            sink.write_little_endian<header::integer, header::size>(header_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return digest == other.digest
                && point == other.point
                && signature == other.signature
                && pair == other.pair
                && group == other.group
                && header_fk == other.header_fk;
        }

        /// pair: m (row 0, for all), n (row 1, for n > 1).
        system::hash_digest digest{};
        system::ec_compressed point{};
        system::ec_signature signature{};
        uint8_t pair{};
        uint16_t group{};
        header::integer header_fk{};
    };

    /// Writer for one row single-sig (0|0 packed in one row, 1-of-1).
    struct put_single_ref
      : public schema::ecdsa
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        /// Writer used for single-sig row, should always write pair = 0.
        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(digest);
            sink.write_bytes(point);
            sink.write_bytes(signature);
            sink.write_byte(0);
            sink.write_little_endian<uint16_t>(group);
            sink.write_little_endian<header::integer, header::size>(header_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::hash_digest& digest;
        const system::ec_compressed& point;
        const system::ec_signature& signature;
        uint16_t group{};
        header::integer header_fk{};
    };

    /// Writer for 1-of-1 multisig (0|0 packed in one row).
    /// Writer for multisig groups (sig|key packed in each row).
    struct put_multiple_ref
      : public schema::ecdsa
    {
        // Terminal count fails the write attempt, so to_data() is guarded.
        inline link count() const NOEXCEPT
        {
            using namespace system;
            const auto m = sigs.size();
            const auto n = keys.size();
            const auto gap = n - m;
            if (is_subtract_overflow(n, m) || is_add_overflow(gap, one))
                return {};

            const auto sum = add1(gap);
            if (is_multiply_overflow(m, sum))
                return {};

            return possible_narrow_cast<link::integer>(m * sum);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            constexpr auto max = power2(to_half(byte_bits));
            const auto m = sigs.size();
            const auto n = keys.size();
            if (is_zero(m) || is_zero(n) || n > max || m > n)
                return false;

            for (size_t sig{}; sig < m; ++sig)
            {
                for (auto key = sig; key <= n - (m - sig); ++key)
                {
                    sink.write_bytes(digest);
                    sink.write_bytes(keys.at(key));
                    sink.write_bytes(sigs.at(sig));
                    sink.write_byte(pack_word<uint8_t>(sig, key));
                    sink.write_little_endian<uint16_t>(group);
                    sink.write_little_endian<header::integer, header::size>(
                        header_fk);
                }
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const hash_digest& digest;
        const system::ec_compresseds& keys;
        const system::ec_signatures& sigs;
        const uint16_t group{};
        const header::integer header_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
