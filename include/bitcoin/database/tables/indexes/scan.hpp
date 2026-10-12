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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_SCAN_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_SCAN_HPP

#include <filesystem>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

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

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(tx_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        tx::integer tx_fk{};
    };
};

/// scan_point is an array of silent payment transaction points.
struct scan_point
  : public no_map<schema::scan_point>
{
    using no_map<schema::scan_point>::nomap;

    struct put_ref
      : public schema::scan_point
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(point);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::ec_compressed& point;
    };
};

/// scan_outputs is an array of silent payment transaction prefix spans.
struct scan_outputs
  : public no_map<schema::scan_outputs>
{
    using prefix = schema::scan_prefix::link;
    using no_map<schema::scan_outputs>::nomap;

    struct record
      : public schema::scan_outputs
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            first = source.read_little_endian<prefix::integer, prefix::size>();
            outputs = source.read_little_endian<uint32_t, schema::index>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<prefix::integer, prefix::size>(first);
            sink.write_little_endian<uint32_t, schema::index>(outputs);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        prefix::integer first{};
        uint32_t outputs{};
    };
};

/// scan_prefix is an array of silent payment output key prefixes.
struct scan_prefix
  : public no_map<schema::scan_prefix>
{
    using integral = unsigned_type<schema::prefix>;
    using bytes = system::data_array<schema::prefix>;
    using no_map<schema::scan_prefix>::nomap;

    struct put_refs
      : public schema::scan_prefix
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

    struct put_span
      : public schema::scan_prefix
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(prefixes.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& prefix: prefixes)
                sink.write_bytes(prefix);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const std::span<const bytes> prefixes;
    };
};

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <template <size_t...> class Storage>
using scan_files = mmaps
<
    Storage,
    scan_correlate,
    scan_point,
    scan_outputs
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

using scan_table = arraymaps
<
    scan_correlate::link,
    schema::scan_correlate::align,
    scan_correlate,
    scan_point,
    scan_outputs
>;

template <template <size_t...> class Storage>
class scan
  : public scan_table
{
public:
    scan(database::storage& head, scan_storage<Storage>& body,
        const scan_correlate::link& buckets) NOEXCEPT
      : scan_table(head, body, buckets),
        correlate(*this),
        point(*this),
        outputs(*this)
    {
    }

    column<scan_table, 0> correlate;
    column<scan_table, 1> point;
    column<scan_table, 2> outputs;
};

static_assert(is_same_type<scan_correlate::span,
    system::scan::batch::tx_link>);
static_assert(is_same_type<scan_point::span, system::ec_compressed>);
static_assert(sizeof(system::scan::batch::outputs_t) == scan_outputs::width);
static_assert(is_same_type<scan_prefix::bytes, system::scan::batch::prefix>);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
