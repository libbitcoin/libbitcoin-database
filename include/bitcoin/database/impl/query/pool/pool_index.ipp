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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_INDEX_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_INDEX_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// The pool index is rebuilt from the compacted pool. Retained rows are
// confirmable, so none has a stored cause of absence.
TEMPLATE
bool CLASS::set_pool_index() NOEXCEPT
{
    if (!store_.pool.enabled())
        return true;

    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    constexpr auto none = algorithm::none;

    if (!store_.pool_cells.truncate(0) ||
        !store_.pool_children.truncate(0) ||
        !store_.spends_rows.truncate(0) ||
        !store_.spends_siblings.truncate(0) ||
        !store_.spends_rivals.truncate(0))
        return false;

    const auto count = store_.pool.count().value;
    for (pool_link::integer row{}; row < count; ++row)
    {
        const tx_link tx{ store_.pool.get_key(row) };
        table::pool_edge edge{};
        if (!store_.pool.edges.get(row, edge))
            return false;

        const auto inputs = edge.inputs;
        table::spends::get_refs::parents prevouts(inputs);
        table::spends::get_refs spent{ {}, prevouts };
        if (!store_.spends.get(edge.spends_fk, spent))
            return false;

        pool_links mates(inputs, none);
        pool_links parents(inputs, none);
        for (uint32_t at{}; at < inputs; ++at)
        {
            using prevout = table::prevout::slab_get;
            const tx_link parent{ prevout::output_tx_fk(prevouts.at(at)) };
            const auto parent_row = store_.pool.first(parent);
            if (!parent_row.is_terminal())
                parents.at(at) = possible_narrow_cast<link_t>(parent_row.value);

            bool conflict{};
            auto& mate = mates.at(at);
            const auto point = get_point_key(to_point(tx, at));
            if (!get_pooled_spender(mate, conflict, point, tx, true))
                return false;
        }

        if (!set_pool_links(row, edge.spends_fk, parents, mates))
            return false;
    }

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    system::pool::settle(columns);
    system::pool::label(columns);
    return true;
}

// protected
// The row's cell and children head, and each input's parent row, sibling
// (joining the list of inputs spending its parent) and rival (joining the
// ring of inputs spending its point at a pooled spender of it).
TEMPLATE
bool CLASS::set_pool_links(system::pool::algorithm::link_t row,
    system::pool::algorithm::link_t start,
    const pool_links& parents, const pool_links& mates) NOEXCEPT
{
    using namespace system;
    using link_t = system::pool::algorithm::link_t;
    constexpr auto none = system::pool::algorithm::none;
    constexpr auto absent = system::pool::algorithm::absent;
    const auto put = [](auto& head, link_t index, link_t value) NOEXCEPT
    {
        using replace = typename std::decay_t<decltype(head)>::replace;
        return head.set(index, replace{ {}, value });
    };

    if (!put(store_.pool_cells, row, absent) ||
        !put(store_.pool_children, row, none))
        return false;

    const auto inputs = parents.size();
    for (size_t at{}; at < inputs; ++at)
    {
        const auto input = start + possible_narrow_cast<link_t>(at);
        const auto parent = parents.at(at);
        const auto mate = mates.at(at);
        auto sibling = none;
        auto rival = none;

        if (parent != none)
        {
            sibling = store_.pool_children.at(parent);
            if (!put(store_.pool_children, parent, input))
                return false;
        }

        if (mate != none)
        {
            const auto next = store_.spends_rivals.at(mate);
            rival = (next == none) ? mate : next;
            if (!put(store_.spends_rivals, mate, input))
                return false;
        }

        if (!put(store_.spends_rows, input, parent) ||
            !put(store_.spends_siblings, input, sibling) ||
            !put(store_.spends_rivals, input, rival))
            return false;
    }

    return true;
}

// protected
// Pooled spenders of the tx (the children of a reorganized parent) take it
// as their parent row and join its children list.
TEMPLATE
bool CLASS::set_pool_adoption(system::pool::algorithm::link_t row,
    const transaction& tx) NOEXCEPT
{
    using namespace system;
    using link_t = system::pool::algorithm::link_t;
    constexpr auto none = system::pool::algorithm::none;
    const auto put = [](auto& head, link_t index, link_t value) NOEXCEPT
    {
        using replace = typename std::decay_t<decltype(head)>::replace;
        return head.set(index, replace{ {}, value });
    };

    const auto outputs = tx.outputs_ptr()->size();
    const auto txid = tx.get_hash(false);
    for (uint32_t index{}; index < outputs; ++index)
    {
        for (const auto& in: to_spenders(point{ txid, index }))
        {
            const auto child = to_input_tx(in);
            const auto child_row = store_.pool.first(child);
            if (child_row.is_terminal())
                continue;

            table::pool_edge edge{};
            if (!store_.pool.edges.get(child_row, edge))
                return false;

            const auto position = in - to_point(child, zero).value;
            const auto offset = possible_narrow_cast<link_t>(position);
            const auto input = edge.spends_fk + offset;

            if (store_.spends_rows.at(input) != none)
                continue;

            const auto sibling = store_.pool_children.at(row);
            if (!put(store_.spends_rows, input, row) ||
                !put(store_.spends_siblings, input, sibling) ||
                !put(store_.pool_children, row, input))
                return false;
        }
    }

    return true;
}

// protected
// The index heads hold no count cell, so each is a plain array of cells.
TEMPLATE
bool CLASS::get_pool_columns(system::pool::algorithm::columns_t& out,
    pool_guards& guards) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    using edges_t = algorithm::edges_t;
    using totals_t = algorithm::totals_t;

    const auto rows = std::min
    ({
        possible_narrow_cast<size_t>(store_.pool.count().value),
        possible_narrow_cast<size_t>(store_.pool_cells.count().value),
        possible_narrow_cast<size_t>(store_.pool_children.count().value)
    });

    const auto inputs = std::min
    ({
        possible_narrow_cast<size_t>(store_.spends.count().value),
        possible_narrow_cast<size_t>(store_.spends_rows.count().value),
        possible_narrow_cast<size_t>(store_.spends_siblings.count().value),
        possible_narrow_cast<size_t>(store_.spends_rivals.count().value)
    });

    if (!guards.pool && is_nonzero(rows))
        guards.pool = store_.pool.get_memory();

    if (!guards.cells && is_nonzero(rows))
        guards.cells = store_.pool_cells.get_memory();

    if (!guards.children && is_nonzero(rows))
        guards.children = store_.pool_children.get_memory();

    if (!guards.rows && is_nonzero(inputs))
        guards.rows = store_.spends_rows.get_memory();

    if (!guards.siblings && is_nonzero(inputs))
        guards.siblings = store_.spends_siblings.get_memory();

    if (!guards.rivals && is_nonzero(inputs))
        guards.rivals = store_.spends_rivals.get_memory();

    if (is_nonzero(rows) &&
        (!guards.pool || !guards.cells || !guards.children))
        return false;

    if (is_nonzero(inputs) &&
        (!guards.rows || !guards.siblings || !guards.rivals))
        return false;

    // A null accessor (empty table) has no data.
    const auto data = [](const memory& guard) NOEXCEPT
    {
        link_t* cells{};
        if (guard)
            cells = pointer_cast<link_t>(guard.data());

        return cells;
    };

    const auto raw_edges = store_.pool.edges.get_raw();
    const auto raw_totals = store_.pool.totals.get_raw();
    const auto raw_flags = store_.pool.flags.get_raw();
    const auto raw_heights = store_.pool.heights.get_raw();
    const auto raw_medians = store_.pool.medians.get_raw();
    const auto edges = pointer_cast<const edges_t>(raw_edges);
    const auto totals = pointer_cast<const totals_t>(raw_totals);
    out =
    {
        { data(guards.cells), rows },
        { data(guards.children), rows },
        { edges, rows },
        { totals, rows },
        { data(guards.rows), inputs },
        { data(guards.siblings), inputs },
        { data(guards.rivals), inputs },
        { pointer_cast<const uint32_t>(raw_flags), rows },
        { pointer_cast<const uint32_t>(raw_heights), rows },
        { pointer_cast<const uint32_t>(raw_medians), rows }
    };

    return true;
}

// protected
// A point archived with one spender is not in the duplicate table.
TEMPLATE
bool CLASS::get_pooled_spender(system::pool::algorithm::link_t& out,
    bool& conflicted, const point& point, const tx_link& self,
    bool archived) const NOEXCEPT
{
    using namespace system;
    using link_t = system::pool::algorithm::link_t;
    constexpr auto none = system::pool::algorithm::none;
    out = none;
    conflicted = false;

    if (archived && !store_.duplicate.exists(point))
        return true;

    for (const auto& in: to_spenders(point))
    {
        const auto tx = to_input_tx(in);
        if (tx == self)
            continue;

        if (is_confirmed_tx(tx))
        {
            conflicted = true;
            continue;
        }

        if (out != none)
            continue;

        const auto row = store_.pool.first(tx);
        if (row.is_terminal())
            continue;

        table::pool_edge edge{};
        if (!store_.pool.edges.get(row, edge))
            return false;

        const auto position = in - to_point(tx, zero).value;
        out = edge.spends_fk + possible_narrow_cast<link_t>(position);
    }

    return true;
}

// protected
// The first tx is the coinbase, which spends nothing.
TEMPLATE
bool CLASS::get_pool_conflicts(tx_links& out,
    const tx_links& txs) const NOEXCEPT
{
    out.clear();
    if (is_zero(store_.duplicate.body_size()))
        return true;

    for (auto tx = std::next(txs.cbegin()); tx != txs.cend(); ++tx)
    {
        table::transaction::get_puts record{};
        if (!store_.tx.get(*tx, record))
            return false;

        // Point links are contiguous (computed).
        const auto end = record.points_fk + record.ins_count;
        for (auto fk = record.points_fk; fk < end; ++fk)
            if (!get_doubles(out, get_point_key(fk)))
                return false;
    }

    return true;
}

TEMPLATE
bool CLASS::is_pool_conflicted(bool& out, const tx_link& link,
    const pool_link& row) const NOEXCEPT
{
    out = false;
    table::pool_edge edge{};
    if (!store_.pool.edges.get(row, edge))
        return false;

    for (uint32_t at{}; !out && at < edge.inputs; ++at)
    {
        system::pool::algorithm::link_t mate{};
        const auto point = get_point_key(to_point(link, at));
        if (!get_pooled_spender(mate, out, point, link, true))
            return false;
    }

    return true;
}

// The conflicts of the block's txs are in its prevout cache, which the prune
// leaves in place until the index is settled to the top, and otherwise are
// taken from the archive (a block that was not validated, or was pruned
// before a reorganization). They include the block's own txs, as each spends
// a conflicted point. A conflict is cleared at a reorganization only if no
// other confirmed tx spends a point of the row.
TEMPLATE
bool CLASS::set_pool_block(const header_link& link, bool confirmed) NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    if (!store_.pool.enabled())
        return true;

    // ========================================================================
    const auto scope = get_transactor();

    const auto settled = confirmed ? link : to_parent(link);
    const auto count = store_.pool.count().value;
    if (is_zero(count))
    {
        store_.set_pool_settled(settled);
        return true;
    }

    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || txs.tx_fks.empty())
        return false;

    pool_links block{};
    for (const auto fk: txs.tx_fks)
    {
        const auto row = store_.pool.first(tx_link{ fk });
        if (!row.is_terminal() && row.value < count)
            block.push_back(possible_narrow_cast<link_t>(row.value));
    }

    table::prevout::slab_get cache{};
    if (!store_.prevout.at(to_prevout(link), cache) &&
        !get_pool_conflicts(cache.conflicts, txs.tx_fks))
        return false;

    auto own = txs.tx_fks;
    std::ranges::sort(own);
    pool_links conflicts{};
    for (const auto conflict: cache.conflicts)
    {
        if (std::ranges::binary_search(own, conflict))
            continue;

        const tx_link tx{ conflict };
        const auto row = store_.pool.first(tx);
        if (row.is_terminal() || row.value >= count)
            continue;

        // A row still spent by another confirmed tx keeps its conflict.
        auto still = false;
        if (!confirmed && !is_pool_conflicted(still, tx, row))
            return false;

        if (!still)
            conflicts.push_back(possible_narrow_cast<link_t>(row.value));
    }

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    const auto beyond = [&](link_t row) NOEXCEPT
    {
        return row >= columns.cells.size();
    };

    std::erase_if(block, beyond);
    std::erase_if(conflicts, beyond);
    const auto complete = confirmed ?
        algorithm::confirm(columns, block, conflicts) :
        algorithm::unconfirm(columns, block, conflicts);

    // A component over the member limit is labeled by the columns.
    if (!complete)
        system::pool::label(columns);

    store_.set_pool_settled(settled);
    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
