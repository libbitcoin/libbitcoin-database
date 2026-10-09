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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_SILENT_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_SILENT_HPP

#include <filesystem>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// scan_row is an array of silent payment prefix|point rows.
struct scan_row
  : public no_map<schema::scan_row>
{
    using integral = unsigned_type<schema::prefix>;
    using no_map<schema::scan_row>::nomap;

    struct put_ref
      : public schema::scan_row
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
            {
                sink.write_little_endian<integral>(prefix);
                sink.write_bytes(compressed);
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const std::vector<integral>& prefixes;
        const system::ec_compressed& compressed;
    };
};

/// scan_correlate is an array of silent payment correlation tx fks.
struct scan_correlate
  : public no_map<schema::scan_correlate>
{
    using tx = schema::transaction::link;
    using no_map<schema::scan_correlate>::nomap;

    struct record
      : public schema::scan_correlate
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
      : public schema::scan_correlate
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
using scan_files = mmaps
<
    Storage,
    scan_correlate,
    scan_row
>;

template <template <size_t...> class Storage>
class scan_storage
  : public scan_files<Storage>
{
public:
    scan_storage(const std::filesystem::path& path,
        const storage_settings& settings, bool random_access,
        bool staged=false) NOEXCEPT
      : scan_files<Storage>(path, settings, random_access, staged)
    {
    }
};

/// Aggregate (table)
/// ---------------------------------------------------------------------------

using scan_table = nomaps
<
    scan_correlate::link,
    scan_correlate,
    scan_row
>;

template <template <size_t...> class Storage>
class scan
  : public scan_table
{
public:
    scan(database::storage& head, scan_storage<Storage>& body) NOEXCEPT
      : scan_table(head, body),
        correlate(*this),
        row(*this)
    {
    }

    column<scan_table, 0> correlate;
    column<scan_table, 1> row;
};

static_assert(sizeof(system::scan::batch::row_t) == scan_row::width);

/// silent_row is an array of silent payment prefix|sum|hash batch rows.
struct silent_row
  : public no_map<schema::silent_row>
{
    using integral = scan_row::integral;
    using no_map<schema::silent_row>::nomap;

    struct put_ref
      : public schema::silent_row
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(prefixes.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& prefix: prefixes)
            {
                sink.write_little_endian<integral>(prefix);
                sink.write_bytes(sum);
                sink.write_bytes(hash);
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const std::vector<integral>& prefixes;
        const system::ec_compressed& sum;
        const system::ec_secret& hash;
    };
};

/// silent_correlate is an array of silent payment batch tx|header fks.
struct silent_correlate
  : public no_map<schema::silent_correlate>
{
    using tx = schema::transaction::link;
    using hd = schema::header::link;
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
            header_fk = source.read_little_endian<hd::integer, hd::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        tx::integer tx_fk{};
        hd::integer header_fk{};
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
            {
                sink.write_little_endian<tx::integer, tx::size>(tx_fk);
                sink.write_little_endian<hd::integer, hd::size>(header_fk);
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const tx::integer tx_fk{};
        const hd::integer header_fk{};
    };
};

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <template <size_t...> class Storage>
using silent_files = mmaps
<
    Storage,
    silent_correlate,
    silent_row
>;

template <template <size_t...> class Storage>
class silent_storage
  : public silent_files<Storage>
{
public:
    silent_storage(const std::filesystem::path& path,
        const storage_settings& settings, bool random_access,
        bool staged=false) NOEXCEPT
      : silent_files<Storage>(path, settings, random_access, staged)
    {
    }
};

/// Aggregate (table)
/// ---------------------------------------------------------------------------

using silent_table = nomaps
<
    silent_correlate::link,
    silent_correlate,
    silent_row
>;

template <template <size_t...> class Storage>
class silent
  : public silent_table
{
public:
    silent(database::storage& head, silent_storage<Storage>& body) NOEXCEPT
      : silent_table(head, body),
        correlate(*this),
        row(*this)
    {
    }

    column<silent_table, 0> correlate;
    column<silent_table, 1> row;
};

static_assert(sizeof(system::silent::batch::row_t) ==
    silent_row::width);

/// silent_bk is a record arraymap of silent payment block indexation, indexed
/// by header.fk.
struct silent_bk
  : public array_map<schema::silent_bk>
{
    using array_map<schema::silent_bk>::arraymap;

    struct record
      : public schema::silent_bk
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            indexed = source.read_byte();
            BC_ASSERT(!source || source.get_read_position() == count() * minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_byte(indexed);
            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        uint8_t indexed{};
    };
};
static_assert(is_same_type<scan_correlate::span,
    system::scan::batch::tx_link>);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
