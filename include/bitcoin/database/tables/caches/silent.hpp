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

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(tx_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        tx::integer tx_fk{};
    };
};

/// silent_sum is an array of silent payment batch input key sums.
struct silent_sum
  : public no_map<schema::silent_sum>
{
    using no_map<schema::silent_sum>::nomap;

    struct put_ref
      : public schema::silent_sum
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(sum);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::ec_compressed& sum;
    };
};

/// silent_hash is an array of silent payment batch input hashes.
struct silent_hash
  : public no_map<schema::silent_hash>
{
    using no_map<schema::silent_hash>::nomap;

    struct put_ref
      : public schema::silent_hash
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(hash);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::ec_secret& hash;
    };
};

/// silent_outputs is an array of silent payment batch tx prefix spans.
struct silent_outputs
  : public no_map<schema::silent_outputs>
{
    using prefix = schema::silent_prefix::link;
    using no_map<schema::silent_outputs>::nomap;

    struct record
      : public schema::silent_outputs
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

/// silent_prefix is an array of silent payment batch output key prefixes.
struct silent_prefix
  : public no_map<schema::silent_prefix>
{
    using integral = unsigned_type<schema::prefix>;
    using bytes = system::data_array<schema::prefix>;
    using no_map<schema::silent_prefix>::nomap;

    struct put_refs
      : public schema::silent_prefix
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(prefixes.size());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            for (const auto& prefix: prefixes)
                sink.write_little_endian<integral>(prefix);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const std::vector<integral>& prefixes;
    };
};

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <template <size_t...> class Storage>
using silent_files = mmaps
<
    Storage,
    silent_correlate,
    silent_sum,
    silent_hash,
    silent_outputs
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
    silent_sum,
    silent_hash,
    silent_outputs
>;

template <template <size_t...> class Storage>
class silent
  : public silent_table
{
public:
    silent(database::storage& head, silent_storage<Storage>& body) NOEXCEPT
      : silent_table(head, body),
        correlate(*this),
        sum(*this),
        hash(*this),
        outputs(*this)
    {
    }

    column<silent_table, 0> correlate;
    column<silent_table, 1> sum;
    column<silent_table, 2> hash;
    column<silent_table, 3> outputs;
};

static_assert(is_same_type<silent_sum::span, system::ec_compressed>);
static_assert(is_same_type<silent_hash::span, system::ec_secret>);
static_assert(sizeof(system::scan::batch::outputs_t) ==
    silent_outputs::width);
static_assert(is_same_type<silent_prefix::bytes,
    system::scan::batch::prefix>);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
