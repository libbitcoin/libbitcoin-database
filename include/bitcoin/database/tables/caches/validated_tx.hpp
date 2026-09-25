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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_TX_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_TX_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Spine column (search key and validation state).
struct validated_tx_state
{
    using link = schema::validated_tx::link;
    static constexpr auto width = schema::validated_tx::minrow;
    static constexpr auto suffix = "state"_t;
};

/// One 64 bit word of the short id hash (common to the id columns).
struct validated_tx_word
  : public schema::validated_tx_id0
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

    inline bool operator==(const validated_tx_word&) const NOEXCEPT = default;

    uint64_t word{};
};

/// Transaction identifier columns (words 0-3 of the short id hash).
struct validated_tx_id0
  : public no_map<schema::validated_tx_id0>
{
    using no_map<schema::validated_tx_id0>::nomap;
};

struct validated_tx_id1
  : public no_map<schema::validated_tx_id1>
{
    using no_map<schema::validated_tx_id1>::nomap;
};

struct validated_tx_id2
  : public no_map<schema::validated_tx_id2>
{
    using no_map<schema::validated_tx_id2>::nomap;
};

struct validated_tx_id3
  : public no_map<schema::validated_tx_id3>
{
    using no_map<schema::validated_tx_id3>::nomap;
};

/// validated_tx is a record hashmap of pool tx validation state keyed by tx
/// link, with the short id hash (txid or wtxid) as four word columns.
struct validated_tx
  : public hash_maps<schema::validated_tx, validated_tx_id0,
      validated_tx_id1, validated_tx_id2, validated_tx_id3>
{
    using base = hash_maps<schema::validated_tx, validated_tx_id0,
        validated_tx_id1, validated_tx_id2, validated_tx_id3>;
    using sigop = linkage<schema::sigops>;
    using spend = schema::spends::link;
    using base::hashmaps;

    struct record
      : public schema::validated_tx
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

    /// The identifier word columns (rows aligned with the spine).
    column<base, 1> id0{ *this };
    column<base, 2> id1{ *this };
    column<base, 3> id2{ *this };
    column<base, 4> id3{ *this };
};

/// Aggregate (files).
template <template <size_t...> class Storage>
using validated_tx_storage = mmaps<Storage, validated_tx_state,
    validated_tx_id0, validated_tx_id1, validated_tx_id2, validated_tx_id3>;

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
