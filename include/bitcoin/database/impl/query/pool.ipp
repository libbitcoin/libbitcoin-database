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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Transaction pool.
// ----------------------------------------------------------------------------

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
bool CLASS::is_confirmable_tx(const tx_link& link) const NOEXCEPT
{
    confirmables memo{};
    return is_confirmable_tx(memo, link);
}

TEMPLATE
bool CLASS::is_confirmable_spent(const output_link& link) const NOEXCEPT
{
    for (const auto& in: to_spenders(link))
    {
        const auto tx = to_input_tx(in);
        if (is_pooled(tx) && is_confirmable_tx(tx))
            return true;
    }

    return false;
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

    table::spends::get_refs::parents parents(out.prevouts.size());
    table::spends::get_refs run{ {}, parents };
    if (!store_.spends.get(valid.spends_fk, run))
        return error::integrity;

    out.fee = valid.fee;
    out.sigops = valid.sigops;
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

// A terminal cursor starts at the first record, and the cursor is set to end
// once the records are exhausted.
TEMPLATE
code CLASS::get_confirmable_txs(pool_link& cursor, tx_links& out,
    const pool_link& end, size_t limit) const NOEXCEPT
{
    out.clear();
    auto fk = cursor.is_terminal() ? pool_link::integer{} : cursor.value;
    if (fk > end)
        return error::invalid_cursor;

    if (!store_.pool.enabled())
    {
        cursor = end;
        return error::success;
    }

    confirmables memo{};
    const auto stop = std::min(end.value, store_.pool.count().value);
    for (; fk < stop && out.size() < limit; ++fk)
    {
        const auto link = to_pool_tx(fk);
        if (!link.is_terminal() && is_confirmable_tx(memo, link))
            out.push_back(link);
    }

    cursor = fk == stop ? end : pool_link{ fk };
    return error::success;
}

TEMPLATE
code CLASS::get_confirmable_txs(tx_links& out) const NOEXCEPT
{
    out.clear();
    if (!store_.pool.enabled())
        return error::success;

    pool_link cursor{};
    return get_confirmable_txs(cursor, out, store_.pool.count(), max_size_t);
}

// An unconfirmed parent of a confirmable tx is itself confirmable.
TEMPLATE
tx_links CLASS::to_confirmable_parents(const tx_link& link) const NOEXCEPT
{
    tx_links out{};
    for (const auto& point: to_points(link))
    {
        const auto parent = to_tx(get_point_key(point).hash());
        if (!parent.is_terminal() && !is_confirmed_tx(parent) &&
            is_pooled(parent) && !system::contains(out, parent.value))
            out.push_back(parent.value);
    }

    return out;
}

TEMPLATE
tx_links CLASS::to_confirmable_children(const tx_link& link) const NOEXCEPT
{
    tx_links out{};
    for (const auto& output: to_outputs(link))
    {
        for (const auto& in: to_spenders(output))
        {
            const auto child = to_input_tx(in);
            if (!child.is_terminal() && is_pooled(child) &&
                is_confirmable_tx(child) &&
                !system::contains(out, child.value))
                out.push_back(child.value);
        }
    }

    return out;
}

TEMPLATE
bool CLASS::get_ancestor_totals(fee_totals& out,
    const tx_link& link) const NOEXCEPT
{
    return get_confirmable_totals(out, link, true);
}

TEMPLATE
bool CLASS::get_descendant_totals(fee_totals& out,
    const tx_link& link) const NOEXCEPT
{
    return get_confirmable_totals(out, link, false);
}

TEMPLATE
bool CLASS::get_confirmable_fees(fee_rates& out) const NOEXCEPT
{
    out.clear();
    tx_links links{};
    if (get_confirmable_txs(links))
        return false;

    out.resize(links.size());
    auto rate = out.begin();
    for (const auto& link: links)
        if (!get_tx_fees(*rate++, link))
            return false;

    return true;
}

// The block's own txs are excluded.
TEMPLATE
bool CLASS::get_block_conflict_fees(fee_rates& out,
    const header_link& link) const NOEXCEPT
{
    out.clear();
    if (!store_.pool.enabled())
        return true;

    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    const std::unordered_set<tx_link::integer> block
    {
        txs.cbegin(), txs.cend()
    };

    std::vector<tx_link::integer> pending{};
    std::unordered_set<tx_link::integer> visited{};
    const auto add = [&](const tx_link& tx) NOEXCEPT
    {
        if (!block.contains(tx.value) && is_pooled(tx) &&
            visited.insert(tx.value).second)
            pending.push_back(tx.value);
    };

    for (const auto& point: to_points(txs))
        for (const auto& in: to_spenders(get_point_key(point)))
            add(to_input_tx(in));

    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();
        if (!get_tx_fees(out.emplace_back(), next))
            return false;

        for (const auto& output: to_outputs(next))
            for (const auto& in: to_spenders(output))
                add(to_input_tx(in));
    }

    return true;
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
    if (!store_.pool.enabled())
        return false;

    const auto fk = store_.pool.first(link);
    if (fk.is_terminal())
        return false;

    table::pool::get_fee pooled{};
    if (!store_.pool.get(fk, pooled))
        return false;

    fee = pooled.fee;
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

// A walk that finds no confirmed spend proves each tx visited confirmable.
TEMPLATE
bool CLASS::is_confirmable_tx(confirmables& memo,
    const tx_link& link) const NOEXCEPT
{
    if (const auto it = memo.find(link.value); it != memo.cend())
        return it->second;

    const auto unconfirmable = [&]() NOEXCEPT
    {
        memo.emplace(link.value, false);
        return false;
    };

    if (is_confirmed_tx(link))
        return unconfirmable();

    std::vector<tx_link::integer> pending{ link.value };
    std::unordered_set<tx_link::integer> visited{ link.value };
    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();

        if (const auto it = memo.find(next.value); it != memo.cend())
        {
            if (!it->second)
                return unconfirmable();

            continue;
        }

        for (const auto& point: to_points(next))
        {
            const auto prevout = get_point_key(point);
            if (!find_confirmed_spender(prevout).is_terminal())
                return unconfirmable();

            const auto parent = to_tx(prevout.hash());
            if (!parent.is_terminal() && !is_confirmed_tx(parent) &&
                visited.insert(parent.value).second)
                pending.push_back(parent.value);
        }
    }

    for (const auto tx: visited)
        memo.emplace(tx, true);

    return true;
}

TEMPLATE
bool CLASS::get_confirmable_totals(fee_totals& out, const tx_link& link,
    bool ancestors) const NOEXCEPT
{
    using namespace system;
    out = {};
    std::vector<tx_link::integer> pending{ link.value };
    std::unordered_set<tx_link::integer> visited{ link.value };
    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();

        fee_rate rate{};
        if (!get_tx_fees(rate, next))
            return false;

        ++out.count;
        out.bytes = ceilinged_add(out.bytes, rate.bytes);
        out.fee = ceilinged_add(out.fee, rate.fee);

        const auto links = ancestors ? to_confirmable_parents(next) :
            to_confirmable_children(next);

        for (const auto fk: links)
            if (visited.insert(fk).second)
                pending.push_back(fk);
    }

    return true;
}

// writers
// ----------------------------------------------------------------------------

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
    using sigs = linkage<schema::sigops>;
    const auto& ins = *tx.inputs_ptr();
    tx_links prevouts(ins.size());

    auto it = prevouts.begin();
    for (const auto& in: ins)
    {
        tx_link parent{ in->metadata.parent_tx };
        auto coinbase = in->metadata.coinbase;

        // Prevouts populated from the tx's own package are not yet linked.
        if (in->metadata.parent_tx == max_uint32)
        {
            if ((parent = to_tx(in->point().hash())).is_terminal())
                return false;

            coinbase = is_coinbase(parent);
        }

        *it++ = table::prevout::merge(coinbase, parent);
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
    if (!store_.spends.put_link(first, table::spends::put_refs{ {}, prevouts }))
        return false;

    auto& vtx = store_.pool;
    const auto fk = vtx.allocate(1);
    if (fk.is_terminal())
        return false;

    if (!store_.wtxid.put(fk, hash))
        return false;

    // Column puts are unguarded, the accessor guards their rows against remap.
    using word = table::pool_word;
    auto guard = vtx.get_memory();
    if (!guard ||
        !vtx.id0.put(fk, word{ {}, std::get<0>(words) }) ||
        !vtx.id1.put(fk, word{ {}, std::get<1>(words) }) ||
        !vtx.id2.put(fk, word{ {}, std::get<2>(words) }) ||
        !vtx.id3.put(fk, word{ {}, std::get<3>(words) }))
        return false;

    guard.reset();

    // Commit is deferred until the columns are set.
    return vtx.put(fk, link, table::pool::record
    {
        {},
        ctx,
        tx.fee(),
        possible_narrow_cast<sigs::integer>(sigops),
        first,
        time
    });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
