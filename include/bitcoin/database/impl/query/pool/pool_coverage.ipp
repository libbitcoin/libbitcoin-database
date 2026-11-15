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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_COVERAGE_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_COVERAGE_IPP

#include <unordered_map>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
code CLASS::get_pool_coverage(bool& out,
    const system::chain::transaction_cptrs& txs,
    const std::vector<bool>& stored, const context& ctx, uint64_t rate,
    uint32_t bytes_per_sigop, const pool_totals& limit,
    size_t maximum_outcomes) const NOEXCEPT
{
    out = false;
    if (!store_.pool.enabled())
    {
        out = true;
        return error::success;
    }

    pool_graph graph{};
    if (const auto ec = get_pool_graph(graph, txs, stored, ctx))
        return ec;

    return get_pool_coverage_(out, graph, rate, bytes_per_sigop, limit,
        maximum_outcomes);
}

// protected
// Candidates are rows and inputs numbered from the table counts. A parent in
// the package is found by hash (parents precede children), a pooled parent by
// link, and any other parent is none. An input joins the ring of a pooled
// spender of its point by a bridge from that input, or a ring of candidate
// spenders if there is none.
TEMPLATE
code CLASS::get_pool_graph(pool_graph& out,
    const system::chain::transaction_cptrs& txs,
    const std::vector<bool>& stored, const context& ctx) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    constexpr auto none = algorithm::none;

    algorithm::columns_t table{};
    if (!get_pool_columns(table, out.guards))
        return error::integrity;

    const auto rows = possible_narrow_cast<link_t>(table.cells.size());
    const auto inputs = possible_narrow_cast<link_t>(table.rows.size());
    const auto bip16 = ctx.is_enabled(chain::flags::bip16_rule);
    const auto bip141 = ctx.is_enabled(chain::flags::bip141_rule);
    std::unordered_map<hash_digest, link_t> positions{};
    std::unordered_map<point, link_t> points{};
    auto input = inputs;
    auto row = rows;
    for (size_t index{}; index < txs.size(); ++index)
    {
        if (stored.at(index))
            continue;

        const auto& tx = *txs.at(index);
        const auto& ins = *tx.inputs_ptr();
        const auto count = possible_narrow_cast<link_t>(ins.size());
        const auto sigops = tx.signature_operations(bip16, bip141);
        positions.emplace(tx.get_hash(false), row);
        out.candidates.push_back(row);
        out.cells.push_back(algorithm::absent);
        out.children.push_back(none);
        out.edges.push_back({ input, count });
        out.totals.push_back(
        {
            tx.fee(),
            possible_narrow_cast<uint32_t>(tx.virtual_size()),
            possible_narrow_cast<uint32_t>(sigops)
        });

        for (const auto& in: ins)
        {
            const auto& point = in->point();
            auto parent = none;
            if (in->metadata.parent_tx == max_uint32)
            {
                const auto it = positions.find(point.hash());
                if (it != positions.end())
                    parent = it->second;
            }
            else
            {
                const tx_link parent_tx{ in->metadata.parent_tx };
                const auto fk = store_.pool.first(parent_tx);
                if (!fk.is_terminal())
                    parent = possible_narrow_cast<link_t>(fk.value);
            }

            out.rows.push_back(parent);
            out.siblings.push_back(none);
            out.rivals.push_back(none);

            if (parent != none && parent < rows)
            {
                out.child_bridges.push_back({ parent, input });
            }
            else if (parent != none)
            {
                auto& head = out.children.at(parent - rows);
                out.siblings.back() = head;
                head = input;
            }

            link_t mate{};
            bool conflict{};
            if (!get_pooled_spender(mate, conflict, point, {}, false))
                return error::integrity;

            out.conflicted |= conflict;
            if (mate != none)
            {
                out.rivals.back() = mate;
                out.rival_bridges.push_back({ mate, input });
            }
            else if (const auto it = points.find(point); it == points.end())
            {
                points.emplace(point, input);
            }
            else
            {
                auto& ring = out.rivals.at(it->second - inputs);
                out.rivals.back() = (ring == none) ? it->second : ring;
                ring = input;
            }

            ++input;
        }

        ++row;
    }

    const auto encode = [](auto& column) NOEXCEPT
    {
        for (auto& value: column)
            value = algorithm::to_column(value);
    };

    encode(out.cells);
    encode(out.children);
    encode(out.edges);
    encode(out.totals);
    encode(out.rows);
    encode(out.siblings);
    encode(out.rivals);
    out.graph =
    {
        table,
        {
            out.cells, out.children, out.edges, out.totals,
            out.rows, out.siblings, out.rivals
        },
        out.child_bridges,
        out.rival_bridges
    };

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_pool_coverage_(bool& out, const pool_graph& graph,
    uint64_t rate, uint32_t bytes_per_sigop, const pool_totals& limit,
    size_t maximum_outcomes) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    using mask_t = algorithm::mask_t;
    constexpr auto none = algorithm::none;
    const auto& g = graph.graph;
    const auto rows = possible_narrow_cast<link_t>(g.table.cells.size());
    const auto inputs = possible_narrow_cast<link_t>(g.table.rows.size());
    out = false;

    // A point of the package spent by a confirmed tx cannot be confirmed.
    if (graph.conflicted)
        return error::success;

    // A candidate with an absent parent cannot be confirmed.
    for (const auto candidate: graph.candidates)
    {
        const auto offset = candidate - rows;
        const auto slice = algorithm::from_column(graph.edges.at(offset));
        for (auto at = slice.first; at < slice.first + slice.count; ++at)
        {
            const auto input = at - inputs;
            const auto parent = algorithm::from_column(graph.rows.at(input));
            if (parent == none || parent >= rows)
                continue;

            const auto cell = algorithm::from_column(g.table.cells[parent]);
            if (cell == algorithm::absent)
                return error::success;
        }
    }

    algorithm::initialize(g, graph.candidates);
    uint64_t added{};
    uint64_t bytes{};
    std::vector<algorithm::component_t> components{};
    for (const auto candidate: graph.candidates)
    {
        const std::span<const link_t> self{ &candidate, one };
        const auto offset = candidate - rows;
        const auto totals = algorithm::from_column(graph.totals.at(offset));
        const auto size = algorithm::chargeable(totals, bytes_per_sigop);
        const auto seen = [&](const auto& component) NOEXCEPT
        {
            return is_nonzero(algorithm::members(component, self));
        };

        bytes = ceilinged_add(bytes, size);
        if (std::ranges::any_of(components, seen))
            continue;

        algorithm::component_t component{};
        if (!algorithm::collect(component, g, candidate) ||
            !algorithm::relate(component, g))
            return error::pool_limit;

        pool_totals sum{};
        if (is_nonzero(component.contests) &&
            !algorithm::ancestry(sum, g, component, limit))
            return error::pool_limit;

        const auto outcomes = algorithm::outcomes(component);
        if (outcomes > maximum_outcomes)
            return error::pool_limit;

        std::vector<mask_t> masks(outcomes);
        for (size_t index{}; index < outcomes; ++index)
            masks[index] = algorithm::outcome(component, index);

        uint64_t best{};
        uint64_t best_without{};
        const auto members = algorithm::members(component, graph.candidates);
        algorithm::evaluate(best, best_without, masks, members, component, g);

        added = ceilinged_add(added, floored_subtract(best, best_without));
        components.push_back(component);
    }

    out = algorithm::covered(added, zero, bytes, rate);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
