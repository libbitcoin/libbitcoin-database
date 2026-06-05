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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_MULTISIG_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_MULTISIG_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// multisig is an array of multisig signature validation records.
struct multisig
  : public no_map<schema::multisig>
{
    using header = schema::header::link;
    using no_map<schema::multisig>::nomap;

    struct record
      : public schema::multisig
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            digest = source.read_hash();
            point = source.read_forward<system::ec_compressed_size>();
            signature = source.read_forward<system::ec_signature_size>();
            pair = source.read_byte();
            set = source.read_2_bytes_little_endian();
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
            sink.write_little_endian<uint16_t>(set);
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
                && set == other.set
                && header_fk == other.header_fk;
        }

        system::hash_digest digest{};
        system::ec_compressed point{};
        system::ec_signature signature{};
        uint8_t pair{};
        uint16_t set{};
        header::integer header_fk{};
    };

    struct put_ref
      : public schema::multisig
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            const auto m = sigs.size();
            const auto n = keys.size();

            BC_ASSERT(!is_subtract_overflow(n, m));
            BC_ASSERT(!is_add_overflow(n - m, one));
            BC_ASSERT(!is_multiply_overflow(m, add1(n - m)));
            return possible_narrow_cast<link::integer>(m * add1(n - m));
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            const auto m = sigs.size();
            const auto n = keys.size();
            constexpr auto max = power2(to_half(byte_bits));
            if (is_zero(m) || is_zero(n) || n > max || m > n)
                return false;

            for (size_t sig{}; sig < m; ++sig)
            {
                for (auto key = sig; key <= n - (m - sig); ++key)
                {
                    sink.write_bytes(digest);
                    sink.write_bytes(keys.at(key));
                    sink.write_bytes(sigs.at(sig));
                    sink.write_byte(pack_word<uint8_t>(m, n));
                    sink.write_little_endian<uint16_t>(set);
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
        const uint16_t set{};
        const header::integer header_fk{};
    };
};

static_assert(sizeof(system::multisig::signatures) == schema::multisig::minrow);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
