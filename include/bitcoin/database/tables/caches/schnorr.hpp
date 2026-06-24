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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_SCHNORR_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_SCHNORR_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// DEPRECATED: schnorr is an array of schnorr signature verification records.
struct schnorr
  : public no_map<schema::schnorr>
{
    using header = schema::header::link;
    using category_t = system::chain::threshold::category_t;
    using no_map<schema::schnorr>::nomap;

    struct record
      : public schema::schnorr
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            digest = source.read_hash();
            point = source.read_forward<system::ec_xonly_size>();
            signature = source.read_forward<system::ec_signature_size>();
            category = static_cast<category_t>(source.read_byte());
            pair = source.read_little_endian<uint16_t>();
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
            sink.write_byte(to_value(category));
            sink.write_little_endian<uint16_t>(pair);
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
                && category == other.category
                && pair == other.pair
                && group == other.group
                && header_fk == other.header_fk;
        }

        /// pair: min (row 0, for all), max (row 1, for category::between).
        system::hash_digest digest{};
        system::ec_xonly point{};
        system::ec_signature signature{};
        category_t category{};
        uint16_t pair{};
        uint16_t group{};
        header::integer header_fk{};
    };

    struct put_single_ref
      : public schema::schnorr
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            constexpr auto category = to_value(category_t::single);
            sink.write_bytes(digest);
            sink.write_bytes(point);
            sink.write_bytes(signature);
            sink.write_byte(category);
            sink.write_little_endian<uint16_t>(1);
            sink.write_little_endian<uint16_t>(group);
            sink.write_little_endian<header::integer, header::size>(header_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        /// pair: min (row 0, for all), max (row 1, for category::between).
        const system::hash_digest& digest;
        const system::ec_xonly& point;
        const system::ec_signature& signature;
        uint16_t group{};
        header::integer header_fk{};
    };

    /// Writer for threshold groups (denormalized min/max in first two rows).
    struct put_multiple_ref
      : public schema::schnorr
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(batch.tuples.size());
        }

        /// min in first row, max in second row (for within only).
        inline uint16_t to_pair(size_t index, size_t count, bool between,
            uint16_t min, uint16_t max) const NOEXCEPT
        {
            if (is_zero(index))
                return min;

            if (between && is_one(index) && count > one)
                return max;

            return {};
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto rows = count();
            const auto min = batch.minimum;
            const auto max = batch.maximum;
            const auto cat = batch.category;
            const bool between = (cat == category_t::between);

            for (size_t row{}; row < rows; ++row)
            {
                const auto category = is_zero(row) ? to_value(cat) : 0_u8;
                const auto pair = to_pair(row, rows, between, min, max);
                const auto& tuple = batch.tuples.at(row);

                sink.write_bytes(tuple.digest);
                sink.write_bytes(tuple.point.get());
                sink.write_bytes(tuple.sig.get());
                sink.write_byte(category);
                sink.write_little_endian<uint16_t>(pair);
                sink.write_little_endian<uint16_t>(group);
                sink.write_little_endian<header::integer, header::size>(
                    header_fk);
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::chain::threshold& batch;
        uint16_t group{};
        header::integer header_fk{};
    };
};

/// schnorr_digest is an array of schnorr verification record signature hashes.
struct schnorr_digest
  : public no_map<schema::schnorr_digest>
{
    using tuples_t = system::chain::threshold::tuples_t;
    using no_map<schema::schnorr_digest>::nomap;

    struct put_ref
      : public schema::schnorr_digest
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tuples.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& tuple: tuples)
                sink.write_bytes(tuple.digest);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const tuples_t& tuples;
    };
};

/// schnorr_xonly is an array of schnorr verification xonly public keys.
struct schnorr_xonly
  : public no_map<schema::schnorr_xonly>
{
    using tuples_t = system::chain::threshold::tuples_t;
    using no_map<schema::schnorr_xonly>::nomap;

    struct put_ref
      : public schema::schnorr_xonly
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tuples.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& tuple: tuples)
                sink.write_bytes(tuple.point.get());

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const tuples_t& tuples;
    };
};

/// schnorr_signature is an array of schnorr verification signatures.
struct schnorr_signature
  : public no_map<schema::schnorr_signature>
{
    using tuples_t = system::chain::threshold::tuples_t;
    using no_map<schema::schnorr_signature>::nomap;

    struct put_ref
      : public schema::schnorr_signature
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tuples.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& tuple: tuples)
                sink.write_bytes(tuple.sig.get());

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const tuples_t& tuples;
    };
};

/// schnorr_correlate is an array of schnorr correlation records.
struct schnorr_correlate
  : public no_map<schema::schnorr_correlate>
{
    using header = schema::header::link;
    using category_t = system::chain::threshold::category_t;
    using no_map<schema::schnorr_correlate>::nomap;

    struct record
      : public schema::schnorr_correlate
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            category = static_cast<category_t>(source.read_byte());
            pair = source.read_little_endian<uint16_t>();
            group = source.read_little_endian<uint16_t>();
            header_fk = source.read_little_endian<header::integer, header::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        category_t category{};
        uint16_t pair{};
        uint16_t group{};
        header::integer header_fk{};
    };

    struct put_ref
      : public schema::schnorr_correlate
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(batch.tuples.size());
        }

        /// min in first row, max in second row (for within only).
        inline uint16_t to_pair(size_t index, size_t count, bool between,
            uint16_t min, uint16_t max) const NOEXCEPT
        {
            if (is_zero(index))
                return min;

            if (between && is_one(index) && count > one)
                return max;

            return {};
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto rows = count();
            const auto min = batch.minimum;
            const auto max = batch.maximum;
            const auto cat = batch.category;
            const bool between = (cat == category_t::between);

            for (size_t row{}; row < rows; ++row)
            {
                const auto category = is_zero(row) ? to_value(cat) : 0_u8;
                const auto pair = to_pair(row, rows, between, min, max);

                sink.write_byte(category);
                sink.write_little_endian<uint16_t>(pair);
                sink.write_little_endian<uint16_t>(group);
                sink.write_little_endian<header::integer, header::size>(
                    header_fk);
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::chain::threshold& batch;
        const uint16_t group{};
        const header::integer header_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
