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

/// silent_row is an array of silent payment prefix|sum|hash batch rows.
struct silent_row
  : public no_map<schema::silent_row>
{
    using integral = unsigned_type<schema::prefix>;
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

/// silent_correlate is an array of silent payment batch tx fks.
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

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
