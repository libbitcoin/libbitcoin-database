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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_WRITER_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_WRITER_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::set_pooled(const tx_link& link, const transaction& tx,
    const chain_context& ctx, uint32_t time) NOEXCEPT
{
    return set_pooled(link, tx, context::from(ctx), time);
}

TEMPLATE
bool CLASS::set_pooled(const tx_link& link, const transaction& tx,
    const context& ctx, uint32_t time) NOEXCEPT
{
    if (!store_.pool.enabled())
        return true;

    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    constexpr auto none = algorithm::none;
    const auto& ins = *tx.inputs_ptr();
    const auto inputs = ins.size();
    auto conflicted = false;
    tx_links prevouts(inputs);
    pool_links mates(inputs, none);
    pool_links parents(inputs, none);

    for (size_t at{}; at < inputs; ++at)
    {
        const auto& in = *ins.at(at);
        const auto& point = in.point();
        tx_link parent{ in.metadata.parent_tx };
        auto coinbase = in.metadata.coinbase;

        // Prevouts populated from the tx's own package are not yet linked.
        if (in.metadata.parent_tx == max_uint32)
        {
            if ((parent = to_tx(point.hash())).is_terminal())
                return false;

            coinbase = is_coinbase(parent);
        }

        prevouts.at(at) = table::prevout::merge(coinbase, parent);
        const auto row = store_.pool.first(parent);
        if (!row.is_terminal())
            parents.at(at) = possible_narrow_cast<link_t>(row.value);

        bool conflict{};
        auto& mate = mates.at(at);
        if (!get_pooled_spender(mate, conflict, point, link, true))
            return false;

        conflicted |= conflict;
    }

    const auto bip16 = ctx.is_enabled(chain::flags::bip16_rule);
    const auto bip141 = ctx.is_enabled(chain::flags::bip141_rule);
    const auto sigops = tx.signature_operations(bip16, bip141);
    using lane_t = schema::pool::witness_lane;
    const auto hash = tx.get_hash(true);
    const auto words = from_little_endians(array_cast<lane_t>(hash));

    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    table::spends::link first{};
    const table::spends::put_refs spent{ {}, prevouts };
    if (!store_.spends.put_link(first, spent))
        return false;

    auto& vtx = store_.pool;
    const auto fk = vtx.allocate(1);
    if (fk.is_terminal())
        return false;

    if (!store_.wtxid.put(fk, hash))
        return false;

    // Column puts are unguarded, the accessor guards their rows against remap.
    using word = table::pool_word;
    const table::pool_total total
    {
        {},
        tx.fee(),
        possible_narrow_cast<uint32_t>(tx.virtual_size()),
        possible_narrow_cast<uint32_t>(sigops)
    };
    const table::pool_edge edge
    {
        {},
        first,
        possible_narrow_cast<uint32_t>(inputs)
    };

    auto guard = vtx.get_memory();
    if (!guard ||
        !vtx.id0.put(fk, word{ {}, std::get<0>(words) }) ||
        !vtx.id1.put(fk, word{ {}, std::get<1>(words) }) ||
        !vtx.id2.put(fk, word{ {}, std::get<2>(words) }) ||
        !vtx.id3.put(fk, word{ {}, std::get<3>(words) }) ||
        !vtx.totals.put(fk, total) ||
        !vtx.edges.put(fk, edge))
        return false;

    guard.reset();

    // Commit is deferred until the columns are set.
    if (!vtx.put(fk, link, table::pool::record{ {}, ctx, time }))
        return false;

    const auto row = possible_narrow_cast<link_t>(fk.value);
    const auto start = possible_narrow_cast<link_t>(first.value);
    if (!set_pool_links(row, start, parents, mates) ||
        !set_pool_adoption(row, tx))
        return false;

    // A conflict with a confirmed spender remains absent.
    if (conflicted)
        return true;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    algorithm::admit(columns, row);
    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
