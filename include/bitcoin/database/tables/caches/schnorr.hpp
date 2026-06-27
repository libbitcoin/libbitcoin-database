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

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <typename Storage>
using schnorr_files = maps
<
    Storage,
    schnorr_correlate,
    schnorr_digest,
    schnorr_xonly,
    schnorr_signature
>;

template <typename Storage>
class schnorr_storage
  : public schnorr_files<Storage>
{
public:
    schnorr_storage(const std::filesystem::path& path, size_t size,
        size_t rate, bool random_access) NOEXCEPT
      : schnorr_files<Storage>(path, size, rate, random_access)
    {
    }

    Storage& correlate = std::get<0>(this->files_);
    Storage& digest = std::get<1>(this->files_);
    Storage& xonly = std::get<2>(this->files_);
    Storage& signature = std::get<3>(this->files_);
};

/// Aggregate (table)
/// ---------------------------------------------------------------------------

template <typename Storage>
using schnorr_table = nomaps
<
    Storage,
    schnorr_correlate,
    schnorr_digest,
    schnorr_xonly,
    schnorr_signature
>;

template <typename Storage>
class schnorr
  : public schnorr_table<Storage>
{
public:
    using storage = schnorr_storage<Storage>;

    schnorr(storage& head, storage& body) NOEXCEPT
      : schnorr_table<Storage>(head, body)
    {
    }

    schnorr_correlate& correlate = std::get<0>(this->tables_);
    schnorr_digest& digest = std::get<1>(this->tables_);
    schnorr_xonly& xonly = std::get<2>(this->tables_);
    schnorr_signature& signature = std::get<3>(this->tables_);
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
