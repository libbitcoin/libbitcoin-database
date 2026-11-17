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
// confirmable, so all are present.
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

    std::ranges::fill(columns.cells,
        algorithm::to_column(algorithm::unlabeled));

    pool_links scratch(columns.rows.size());
    algorithm::initialize(columns);
    while (algorithm::propagate(columns, scratch));
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
        { data(guards.rivals), inputs }
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
// The conflicts of the block's txs that are not pooled are in its prevout
// cache, which is absent when the block was confirmed by bypass.
TEMPLATE
bool CLASS::set_pool_block(const header_link& link, const tx_links& txs,
    bool confirmed) NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    if (!store_.pool.enabled())
        return true;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    const auto rows = columns.cells.size();
    if (is_zero(rows))
        return true;

    for (const auto fk: txs)
    {
        const auto row = store_.pool.first(tx_link{ fk });
        if (row.is_terminal() || row.value >= rows)
            continue;

        const auto at = possible_narrow_cast<link_t>(row.value);
        if (confirmed)
            algorithm::confirm(columns, at);
        else
            algorithm::unconfirm(columns, at);
    }

    table::prevout::slab_get cache{};
    if (!store_.prevout.at(to_prevout(link), cache))
        return true;

    for (const auto conflict: cache.conflicts)
    {
        const auto row = store_.pool.first(tx_link{ conflict });
        if (row.is_terminal() || row.value >= rows)
            continue;

        const auto at = possible_narrow_cast<link_t>(row.value);
        if (confirmed)
            algorithm::exclude(columns, at);
        else
            algorithm::revive(columns, at);
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
