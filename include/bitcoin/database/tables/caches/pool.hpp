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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_POOL_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_POOL_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Spine column (search key and validation context).
struct pool_context
{
    using link = schema::pool::link;
    static constexpr auto width = schema::pool::minrow;
    static constexpr auto suffix = "context"_t;
};

/// One 64 bit word of the short id hash (common to the id columns).
struct pool_word
  : public schema::pool_id0
{
    static constexpr link count() NOEXCEPT
    {
        return 1;
    }

    inline bool from_data(reader& source) NOEXCEPT
    {
        word = source.read_little_endian<uint64_t>();
        BC_ASSERT(!source || source.get_read_position() == minrow);
        return source;
    }

    inline bool to_data(flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint64_t>(word);
        BC_ASSERT(!sink || sink.get_write_position() == minrow);
        return sink;
    }

    inline bool operator==(const pool_word&) const NOEXCEPT = default;

    uint64_t word{};
};

/// Transaction identifier columns (words 0-3 of the short id hash).
struct pool_id0
  : public no_map<schema::pool_id0>
{
    using no_map<schema::pool_id0>::nomap;
};

struct pool_id1
  : public no_map<schema::pool_id1>
{
    using no_map<schema::pool_id1>::nomap;
};

struct pool_id2
  : public no_map<schema::pool_id2>
{
    using no_map<schema::pool_id2>::nomap;
};

struct pool_id3
  : public no_map<schema::pool_id3>
{
    using no_map<schema::pool_id3>::nomap;
};

/// pool is a record hashmap of pooled tx validation context keyed by tx
/// link, with the short id hash (txid or wtxid) as four word columns.
struct pool
  : public hash_maps<schema::pool, pool_id0, pool_id1, pool_id2, pool_id3>
{
    using base = hash_maps<schema::pool, pool_id0, pool_id1, pool_id2, pool_id3>;
    using sigop = linkage<schema::sigops>;
    using spend = schema::spends::link;
    using base::hashmaps;

    struct record
      : public schema::pool
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            context::from_data(source, ctx);
            fee = source.read_little_endian<uint64_t>();
            sigops = source.read_little_endian<sigop::integer, sigop::size>();
            spends_fk = source.read_little_endian<spend::integer, spend::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_little_endian<uint64_t>(fee);
            sink.write_little_endian<sigop::integer, sigop::size>(sigops);
            sink.write_little_endian<spend::integer, spend::size>(spends_fk);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        context ctx{};
        uint64_t fee{};
        sigop::integer sigops{};
        spend::integer spends_fk{};
    };

    struct get_fee
      : public schema::pool
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(context::size);
            fee = source.read_little_endian<uint64_t>();
            return source;
        }

        uint64_t fee{};
    };

    /// The identifier word columns (rows aligned with the spine).
    column<base, 1> id0{ *this };
    column<base, 2> id1{ *this };
    column<base, 3> id2{ *this };
    column<base, 4> id3{ *this };
};

/// Aggregate (files).
template <template <size_t...> class Storage>
using pool_storage = mmaps<Storage, pool_context,
    pool_id0, pool_id1, pool_id2, pool_id3>;

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
