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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_SCHNORR_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_SCHNORR_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// schnorr_row is an array of schnorr verification digest|key|signature rows.
struct schnorr_row
  : public no_map<schema::schnorr_row>
{
    using no_map<schema::schnorr_row>::nomap;

    struct put_ref
      : public schema::schnorr_row
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(digest);
            sink.write_bytes(key);
            sink.write_bytes(signature);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::hash_digest& digest;
        const system::ec_xonly& key;
        const system::ec_signature& signature;
    };

    struct put_signatures
      : public schema::schnorr_row
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                sigs.rows().size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& row: sigs.rows())
            {
                sink.write_bytes(row.digest);
                sink.write_bytes(row.point);
                sink.write_bytes(row.signature);
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::chain::schnorr_signatures& sigs;
    };
};

/// schnorr_correlate is an array of schnorr correlation records.
struct schnorr_correlate
  : public no_map<schema::schnorr_correlate>
{
    using hd = schema::header::link;
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
            header_fk = source.read_little_endian<hd::integer, hd::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        hd::integer header_fk{};
    };

    struct put_ref
      : public schema::schnorr_correlate
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<hd::integer, hd::size>(header_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const hd::integer header_fk{};
    };

    struct put_signatures
      : public schema::schnorr_correlate
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                sigs.rows().size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            const auto rows = sigs.rows().size();
            for (size_t row{}; row < rows; ++row)
                sink.write_little_endian<hd::integer, hd::size>(header_fk);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const hd::integer header_fk{};
        const system::chain::schnorr_signatures& sigs;
    };
};

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <template <size_t...> class Storage>
using schnorr_files = mmaps
<
    Storage,
    schnorr_correlate,
    schnorr_row
>;

template <template <size_t...> class Storage>
class schnorr_storage
  : public schnorr_files<Storage>
{
public:
    schnorr_storage(const std::filesystem::path& path,
        const storage_settings& settings, bool random_access,
        bool staged=false) NOEXCEPT
      : schnorr_files<Storage>(path, settings, random_access, staged)
    {
    }
};

/// Aggregate (table)
/// ---------------------------------------------------------------------------

using schnorr_table = nomaps
<
    schnorr_correlate::link,
    schnorr_correlate,
    schnorr_row
>;

template <template <size_t...> class Storage>
class schnorr
  : public schnorr_table
{
public:
    schnorr(database::storage& head, schnorr_storage<Storage>& body) NOEXCEPT
      : schnorr_table(head, body),
        correlate(*this),
        row(*this)
    {
    }

    column<schnorr_table, 0> correlate;
    column<schnorr_table, 1> row;
};

static_assert(sizeof(system::schnorr::batch::correlate_t) ==
    schnorr_correlate::width);
static_assert(sizeof(system::schnorr::batch::row_t) == schnorr_row::width);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
