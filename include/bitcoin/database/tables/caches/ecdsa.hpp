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
    
/// Utilities
/// ---------------------------------------------------------------------------
constexpr auto ecdsa_max = system::power2(to_half(byte_bits));

constexpr bool ecdsa_check(size_t m, size_t n) NOEXCEPT
{
    return !is_zero(m) && !is_zero(n) && !(n > ecdsa_max) && !(m > n);
}

constexpr size_t ecdsa_count(size_t m, size_t n) NOEXCEPT
{
    using namespace system;
    const auto gap = n - m;
    if (is_subtract_overflow(n, m) || is_add_overflow(gap, one))
        return {};

    const auto sum = add1(gap);
    if (is_multiply_overflow(m, sum))
        return {};

    return m * sum;
}

/// DEPRECATED: ecdsa is an array of ecdsa signature validation records.
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
        const uint16_t group{};
        const header::integer header_fk{};
    };

    /// Writer for 1-of-1 multisig (0|0 packed in one row).
    /// Writer for multisig groups (sig|key packed in each row).
    struct put_multiple_ref
      : public schema::ecdsa
    {
        // Terminal count fails the write attempt, so to_data() is guarded.
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                ecdsa_count(sigs.size(), keys.size()));
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto m = sigs.size();
            const auto n = keys.size();
            if (!ecdsa_check(m, n))
                return false;

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
            {
                for (auto key = sig; key <= gap + sig; ++key)
                {
                    sink.write_bytes(digest);
                    sink.write_bytes(keys.at(key));
                    sink.write_bytes(sigs.at(sig));
                    sink.write_byte(system::pack_word<uint8_t>(sig, key));
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

/// ecdsa_digest is an array of ecdsa verification record signature hashes.
struct ecdsa_digest
  : public no_map<schema::ecdsa_digest>
{
    using no_map<schema::ecdsa_digest>::nomap;

    struct put_ref
      : public schema::ecdsa_digest
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                ecdsa_count(sigs, keys));
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            // ecdsa multisig capture is limited to common signature hash.
            for (size_t row{}; row < count(); ++row)
                sink.write_bytes(digest);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t keys{};
        const size_t sigs{};
        const hash_digest& digest;
    };
};

/// ecdsa_compressed is an array of ecdsa verification compressed public keys.
struct ecdsa_compressed
  : public no_map<schema::ecdsa_compressed>
{
    using no_map<schema::ecdsa_compressed>::nomap;

    struct put_ref
      : public schema::ecdsa_compressed
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                ecdsa_count(sigs, keys.size()));
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto m = sigs;
            const auto n = keys.size();
            if (!ecdsa_check(m, n))
                return false;

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
                for (auto key = sig; key <= gap + sig; ++key)
                    sink.write_bytes(keys.at(key));

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::ec_compresseds& keys;
        const size_t sigs{};
    };
};

/// ecdsa_signature is an array of ecdsa verification signatures.
struct ecdsa_signature
  : public no_map<schema::ecdsa_signature>
{
    using no_map<schema::ecdsa_signature>::nomap;

    struct put_ref
      : public schema::ecdsa_signature
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                ecdsa_count(sigs.size(), keys));
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto m = sigs.size();
            const auto n = keys;
            if (!ecdsa_check(m, n))
                return false;

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
                for (auto key = sig; key <= gap + sig; ++key)
                    sink.write_bytes(sigs.at(sig));

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t keys{};
        const system::ec_signatures& sigs;
    };
};

/// ecdsa_correlate is an array of ecdsa correlation records.
struct ecdsa_correlate
  : public no_map<schema::ecdsa_correlate>
{
    using header = schema::header::link;
    using no_map<schema::ecdsa_correlate>::nomap;

    struct record
      : public schema::ecdsa_correlate
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            pair = source.read_byte();
            group = source.read_little_endian<uint16_t>();
            header_fk = source.read_little_endian<header::integer, header::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        uint8_t pair{};
        uint16_t group{};
        header::integer header_fk{};
    };

    struct put_ref
      : public schema::ecdsa_correlate
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                ecdsa_count(sigs, keys));
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto m = sigs;
            const auto n = keys;
            if (!ecdsa_check(m, n))
                return false;

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
            {
                for (auto key = sig; key <= gap + sig; ++key)
                {
                    sink.write_byte(system::pack_word<uint8_t>(sig, key));
                    sink.write_little_endian<uint16_t>(group);
                    sink.write_little_endian<header::integer, header::size>(
                        header_fk);
                }
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t keys{};
        const size_t sigs{};
        const uint16_t group{};
        const header::integer header_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
