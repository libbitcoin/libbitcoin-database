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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_READER_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_READER_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::is_pooled(const tx_link& link) const NOEXCEPT
{
    return store_.pool.enabled() && !store_.pool.first(link).is_terminal();
}

TEMPLATE
bool CLASS::get_pooled_time(uint32_t& out, const tx_link& link) const NOEXCEPT
{
    if (!store_.pool.enabled())
        return false;

    const auto fk = store_.pool.first(link);
    if (fk.is_terminal())
        return false;

    table::pool::get_time pooled{};
    if (!store_.pool.get(fk, pooled))
        return false;

    out = pooled.time;
    return true;
}

TEMPLATE
code CLASS::get_pooled(pooled_tx& out, const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    using prevout = table::prevout::slab_get;
    if (!store_.pool.enabled())
        return error::unvalidated;

    const auto fk = store_.pool.first(link);
    if (fk.is_terminal())
        return error::unvalidated;

    table::pool::record valid{};
    if (!store_.pool.get(fk, valid))
        return error::integrity;

    if (!is_sufficient(ctx, valid.ctx))
        return error::unvalidated;

    table::pool_edge edge{};
    table::pool_total total{};
    if (!store_.pool.totals.get(fk, total) || !store_.pool.edges.get(fk, edge))
        return error::integrity;

    table::spends::get_refs::parents parents(out.prevouts.size());
    table::spends::get_refs run{ {}, parents };
    if (!store_.spends.get(edge.spends_fk, run))
        return error::integrity;

    out.fee = total.fee;
    out.sigops = total.sigops;
    std::ranges::transform(parents, out.prevouts.begin(),
        [](auto merged) NOEXCEPT
        {
            return pooled_tx::prevout
            {
                prevout::output_tx_fk(merged), prevout::coinbase(merged)
            };
        });

    return error::success;
}

// protected
// The first record of a tx is its last committed, so the tx of any earlier or
// uncommitted record (its key does not resolve to it) is terminal.
TEMPLATE
tx_link CLASS::to_pool_tx(const pool_link& link) const NOEXCEPT
{
    const tx_link tx{ store_.pool.get_key(link) };
    return store_.pool.first(tx) == link ? tx : tx_link{};
}

TEMPLATE
bool CLASS::get_pooled_fee(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    size_t height{};
    return get_pooled_fee(out, height, link);
}

TEMPLATE
bool CLASS::get_pooled_fee(uint64_t& fee, size_t& height,
    const tx_link& link) const NOEXCEPT
{
    size_t size{};
    return get_pooled_fee(fee, size, height, link);
}

TEMPLATE
bool CLASS::get_pooled_fee(uint64_t& fee, size_t& size, size_t& height,
    const tx_link& link) const NOEXCEPT
{
    if (!store_.pool.enabled())
        return false;

    const auto fk = store_.pool.first(link);
    if (fk.is_terminal())
        return false;

    table::pool_total total{};
    table::pool::get_height pooled{};
    if (!store_.pool.get(fk, pooled) || !store_.pool.totals.get(fk, total))
        return false;

    fee = total.fee;
    size = total.bytes;
    height = pooled.height;
    return true;
}

TEMPLATE
hash_digest CLASS::get_pooled_wtxid(const table::pool::link& link) const NOEXCEPT
{
    using namespace system;
    table::pool_word id0{}, id1{}, id2{}, id3{};
    if (!store_.pool.id0.get(link, id0) ||
        !store_.pool.id1.get(link, id1) ||
        !store_.pool.id2.get(link, id2) ||
        !store_.pool.id3.get(link, id3))
        return {};

    using lane_t = schema::pool::witness_lane;
    using lanes_t = std_array<lane_t, schema::pool::witness_lanes>;
    const auto words = to_little_endians(lanes_t
    {
        id0.word, id1.word, id2.word, id3.word
    });

    return array_cast<uint8_t>(words);
}

} // namespace database
} // namespace libbitcoin

#endif
