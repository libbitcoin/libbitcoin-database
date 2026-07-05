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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_SILENT_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_SILENT_HPP

#include <algorithm>
#include <filesystem>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// silent_prefix is an array of silent payment record prefixes.
struct silent_prefix
  : public no_map<schema::silent_prefix>
{
    using integral = unsigned_type<width>;
    using no_map<schema::silent_prefix>::nomap;

    struct put_ref
      : public schema::silent_prefix
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(prefixes.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            // The prefix must be read from ec_xonly[0..7] as LE.
            // Disk sequence will be [0..7] (with no byteswap on LE hardware).
            for (const auto& prefix: prefixes)
                sink.write_little_endian<integral>(prefix);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const std::vector<integral>& prefixes;
    };
};

/// silent_compressed is an array of silent payment record compresseds.
struct silent_compressed
  : public no_map<schema::silent_compressed>
{
    using no_map<schema::silent_compressed>::nomap;

    struct put_ref
      : public schema::silent_compressed
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(rows);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (size_t row{}; row < rows; ++row)
                sink.write_bytes(compressed);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const system::ec_compressed& compressed;
    };
};

/// silent_correlate is an array of silent payment correlation tx fks.
struct silent_correlate
  : public no_map<schema::silent_correlate>
{
    using tx = schema::transaction::link;
    using no_map<schema::silent_correlate>::nomap;

    struct record
      : public schema::silent_correlate
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            tx_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        tx::integer tx_fk{};
    };

    struct records
      : public schema::silent_correlate
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(rows);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (size_t row{}; row < rows; ++row)
                sink.write_little_endian<tx::integer, tx::size>(tx_fk);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const tx::integer tx_fk{};
    };
};

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <template <size_t...> class Storage>
using silent_files = mmaps
<
    Storage,
    silent_correlate,
    silent_prefix,
    silent_compressed
>;

template <template <size_t...> class Storage>
class silent_storage
  : public silent_files<Storage>
{
public:
    silent_storage(const std::filesystem::path& path, size_t size,
        size_t rate, bool random_access) NOEXCEPT
      : silent_files<Storage>(path, size, rate, random_access)
    {
    }
};

/// Aggregate (table)
/// ---------------------------------------------------------------------------

using silent_table = nomaps
<
    silent_correlate::link,
    silent_correlate,
    silent_prefix,
    silent_compressed
>;

template <template <size_t...> class Storage>
class silent
  : public silent_table
{
public:
    silent(database::storage& head, silent_storage<Storage>& body) NOEXCEPT
      : silent_table(head, body),
        correlate(*this),
        prefix(*this),
        compressed(*this)
    {
    }

    column<silent_table, 0> correlate;
    column<silent_table, 1> prefix;
    column<silent_table, 2> compressed;
};

static_assert(is_same_type<silent_prefix::span,
    system::silent::batch::prefix>);
static_assert(is_same_type<silent_correlate::span,
    system::silent::batch::tx_link>);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
