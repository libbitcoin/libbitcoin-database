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
    size_t repooled{};
    return set_pooled(repooled, link, tx, ctx, time);
}

// Pooled spender of tx (pooled while tx was confirmed) holds lower row, so it
// and its pooled descendants are purged and pooled after tx.
TEMPLATE
bool CLASS::set_pooled(size_t& repooled, const tx_link& link,
    const transaction& tx, const context& ctx, uint32_t time) NOEXCEPT
{
    using namespace system::chain;
    const auto bip16 = ctx.is_enabled(flags::bip16_rule);
    const auto bip141 = ctx.is_enabled(flags::bip141_rule);
    const auto sigops = tx.signature_operations(bip16, bip141);
    return set_pooled(repooled, link, tx, ctx, time, sigops, true);
}

TEMPLATE
bool CLASS::set_pooled(size_t& repooled, const tx_link& link,
    const transaction& tx, const context& ctx, uint32_t time, size_t sigops,
    bool archived) NOEXCEPT
{
    repooled = zero;
    if (!store_.pool.enabled())
        return true;

    if (!set_pool_row(link, tx, ctx, time, sigops))
        return false;

    tx_links purged{};
    if (archived && (!set_pool_purged(purged, link) ||
        !set_pool_repooled(purged)))
        return false;

    repooled = purged.size();
    return true;
}

// protected
TEMPLATE
bool CLASS::set_pool_row(const tx_link& link, const transaction& tx,
    const context& ctx, uint32_t time, size_t sigops) NOEXCEPT
{
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
    using scalar = table::pool_scalar;
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
        !vtx.edges.put(fk, edge) ||
        !vtx.flags.put(fk, scalar{ {}, ctx.flags }) ||
        !vtx.heights.put(fk, scalar{ {}, ctx.height }) ||
        !vtx.medians.put(fk, scalar{ {}, ctx.mtp }))
        return false;

    guard.reset();

    // Commit is deferred until the columns are set.
    if (!vtx.put(fk, link, table::pool::record{ {}, time }))
        return false;

    const auto row = possible_narrow_cast<link_t>(fk.value);
    const auto start = possible_narrow_cast<link_t>(first.value);
    if (!set_pool_rows(row, add1(row)) ||
        !set_pool_links(start, parents, mates))
        return false;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    // Conflict with confirmed spender is absent for it, and component over
    // member limit is labeled by columns.
    if (!algorithm::admit(columns, row, conflicted))
        system::pool::label(columns);

    return true;
    // ========================================================================
}

// protected
TEMPLATE
bool CLASS::set_pool_purged(tx_links& out, const tx_link& link) NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using row_cell = table::pool_index::replace;
    using integer = pool_link::integer;
    auto& cells = store_.pool_index.cells;
    std::vector<std::pair<integer, tx_link>> found{};
    tx_links pending{ link.value };
    out.clear();

    // ========================================================================
    const auto scope = get_transactor();

    while (!pending.empty())
    {
        const tx_link parent{ pending.back() };
        pending.pop_back();
        const auto outputs = to_outputs(parent).size();
        for (uint32_t index{}; index < outputs; ++index)
        {
            for (const auto& in: to_spenders(parent, index))
            {
                const auto child = to_input_tx(in);
                const auto row = store_.pool.first(child);
                if (row.is_terminal())
                    continue;

                const auto cell = cells.at(row.value);
                const auto purged = algorithm::is_absent(cell) &&
                    is_nonzero(bit_and(cell, algorithm::purged));
                if (purged || algorithm::is_boundary(cell))
                    continue;

                const auto next = algorithm::raised(cell, algorithm::purged);
                if (!cells.set(row.value, row_cell{ {}, next }))
                    return false;

                found.emplace_back(row.value, child);
                pending.push_back(child.value);
            }
        }
    }

    if (found.empty())
        return true;

    std::ranges::sort(found, {}, &std::pair<integer, tx_link>::first);
    for (const auto& item: found)
        out.push_back(item.second.value);

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    system::pool::label(columns);
    return true;
    // ========================================================================
}

// protected
TEMPLATE
bool CLASS::set_pool_repooled(const tx_links& purged) NOEXCEPT
{
    for (const auto value: purged)
    {
        const tx_link link{ value };
        table::pool::record record{};
        table::pool_total total{};
        table::pool_scalar flags{}, height{}, median{};
        const auto row = store_.pool.first(link);
        if (!store_.pool.get(row, record) ||
            !store_.pool.totals.get(row, total) ||
            !store_.pool.flags.get(row, flags) ||
            !store_.pool.heights.get(row, height) ||
            !store_.pool.medians.get(row, median))
            return false;

        const auto tx = get_transaction(link, true);
        if (!tx || !populate_with_metadata(*tx, true, true))
            return false;

        const context ctx{ flags.value, height.value, median.value };
        if (!set_pool_row(link, *tx, ctx, record.time, total.sigops))
            return false;
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
