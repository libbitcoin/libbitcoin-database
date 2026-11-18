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
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Spine column (search key and arrival time).
struct pool_time
{
    using link = schema::pool::link;
    static constexpr auto width = schema::pool::minrow;
    static constexpr auto suffix = "time"_t;
};

/// One lane of the witness hash (common to the id columns).
struct pool_word
  : public schema::pool_id0
{
    using lane = schema::pool::witness_lane;

    static constexpr link count() NOEXCEPT
    {
        return 1;
    }

    inline bool from_data(reader& source) NOEXCEPT
    {
        word = source.read_little_endian<lane>();
        BC_ASSERT(!source || source.get_read_position() == minrow);
        return source;
    }

    inline bool to_data(flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<lane>(word);
        BC_ASSERT(!sink || sink.get_write_position() == minrow);
        return sink;
    }

    inline bool operator==(const pool_word&) const NOEXCEPT = default;

    lane word{};
};

/// Fee, virtual size and sigops of the tx (totals column).
struct pool_total
  : public schema::pool_totals
{
    static constexpr link count() NOEXCEPT
    {
        return 1;
    }

    inline bool from_data(reader& source) NOEXCEPT
    {
        fee = source.read_little_endian<uint64_t>();
        bytes = source.read_little_endian<uint32_t>();
        sigops = source.read_little_endian<uint32_t>();
        BC_ASSERT(!source || source.get_read_position() == minrow);
        return source;
    }

    inline bool to_data(flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint64_t>(fee);
        sink.write_little_endian<uint32_t>(bytes);
        sink.write_little_endian<uint32_t>(sigops);
        BC_ASSERT(!sink || sink.get_write_position() == minrow);
        return sink;
    }

    inline bool operator==(const pool_total&) const NOEXCEPT = default;

    uint64_t fee{};
    uint32_t bytes{};
    uint32_t sigops{};
};

/// First spends record and input count of the tx (edges column).
struct pool_edge
  : public schema::pool_edges
{
    using spend = schema::spends::link;

    static constexpr link count() NOEXCEPT
    {
        return 1;
    }

    inline bool from_data(reader& source) NOEXCEPT
    {
        spends_fk = source.read_little_endian<spend::integer, spend::size>();
        inputs = source.read_little_endian<uint32_t>();
        BC_ASSERT(!source || source.get_read_position() == minrow);
        return source;
    }

    inline bool to_data(flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<spend::integer, spend::size>(spends_fk);
        sink.write_little_endian<uint32_t>(inputs);
        BC_ASSERT(!sink || sink.get_write_position() == minrow);
        return sink;
    }

    inline bool operator==(const pool_edge&) const NOEXCEPT = default;

    spend::integer spends_fk{};
    uint32_t inputs{};
};

/// One word of the validation context (common to the context columns).
struct pool_scalar
  : public schema::pool_flags
{
    static constexpr link count() NOEXCEPT
    {
        return 1;
    }

    inline bool from_data(reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        BC_ASSERT(!source || source.get_read_position() == minrow);
        return source;
    }

    inline bool to_data(flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint32_t>(value);
        BC_ASSERT(!sink || sink.get_write_position() == minrow);
        return sink;
    }

    inline bool operator==(const pool_scalar&) const NOEXCEPT = default;

    uint32_t value{};
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

/// Scan columns (fee, size, sigops and edges), words aligned for lanes.
struct pool_totals
  : public no_map<schema::pool_totals>
{
    using no_map<schema::pool_totals>::nomap;
};

struct pool_edges
  : public no_map<schema::pool_edges>
{
    using no_map<schema::pool_edges>::nomap;
};

/// Context columns (flags, height and median time past of the validation).
struct pool_flags
  : public no_map<schema::pool_flags>
{
    using no_map<schema::pool_flags>::nomap;
};

struct pool_heights
  : public no_map<schema::pool_heights>
{
    using no_map<schema::pool_heights>::nomap;
};

struct pool_medians
  : public no_map<schema::pool_medians>
{
    using no_map<schema::pool_medians>::nomap;
};

/// pool is a record hashmap of pooled tx arrival time keyed by tx link, with
/// the short id hash (txid or wtxid) as four word columns, the totals and
/// edges of the tx as two more, and its validation context as three more.
struct pool
  : public hash_maps<schema::pool, pool_id0, pool_id1, pool_id2, pool_id3,
        pool_totals, pool_edges, pool_flags, pool_heights, pool_medians>
{
    using base = hash_maps<schema::pool, pool_id0, pool_id1, pool_id2,
        pool_id3, pool_totals, pool_edges, pool_flags, pool_heights,
        pool_medians>;
    using base::hashmaps;

    struct record
      : public schema::pool
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            time = source.read_little_endian<uint32_t>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<uint32_t>(time);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        uint32_t time{};
    };

    /// The identifier word columns (rows aligned with the spine).
    column<base, 1> id0{ *this };
    column<base, 2> id1{ *this };
    column<base, 3> id2{ *this };
    column<base, 4> id3{ *this };

    /// The scan columns (rows aligned with the spine).
    column<base, 5> totals{ *this };
    column<base, 6> edges{ *this };

    /// The context columns (rows aligned with the spine).
    column<base, 7> flags{ *this };
    column<base, 8> heights{ *this };
    column<base, 9> medians{ *this };
};

/// Aggregate (files).
template <template <size_t...> class Storage>
using pool_storage = mmaps<Storage, pool_time,
    pool_id0, pool_id1, pool_id2, pool_id3, pool_totals, pool_edges,
    pool_flags, pool_heights, pool_medians>;

/// A value head aligned with a table, one word per row, updated in place.
template <typename Schema>
struct value_head
  : public value_map<Schema>
{
    using base = value_map<Schema>;
    using value = typename Schema::value;
    using base::base;

    /// Replace the value.
    struct replace
      : public Schema
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(Schema::cell);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_little_endian<value, Schema::cell>(value_);
            return sink;
        }

        value value_{};
    };
};

/// pool_cells is the state and component label of each pool row.
using pool_cells = value_head<schema::pool_cells>;

/// pool_children is the first input spending each pool row (none if
/// childless), heading the list of its spending inputs.
using pool_children = value_head<schema::pool_children>;

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
