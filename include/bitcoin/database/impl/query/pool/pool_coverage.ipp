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
    for (size_t index{}; index < txs.size(); ++index)
        if (!stored.at(index))
            if (const auto ec = get_pool_candidate(graph, *txs.at(index), ctx))
                return ec;

    return get_pool_coverage(out, graph, zero, rate, bytes_per_sigop, limit,
        maximum_outcomes);
}

// Candidates are rows and inputs numbered from table counts. Parent among
// candidates is found by hash, pooled parent by link, and any other parent is
// none. Input joins ring of a pooled spender of its point by a bridge from that
// input, or a ring of candidate spenders if there is none.
TEMPLATE
code CLASS::get_pool_candidate(pool_graph& out, const transaction& tx,
    const context& ctx) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    constexpr auto none = algorithm::none;

    auto& table = out.graph.table;
    if (out.candidates.empty() && !get_pool_columns(table, out.guards))
        return error::integrity;

    const auto rows = possible_narrow_cast<link_t>(table.cells.size());
    const auto inputs = possible_narrow_cast<link_t>(table.rows.size());
    const auto row = possible_narrow_cast<link_t>(rows +
        out.candidates.size());

    auto input = possible_narrow_cast<link_t>(inputs + out.rows.size());
    const auto bip16 = ctx.is_enabled(chain::flags::bip16_rule);
    const auto bip141 = ctx.is_enabled(chain::flags::bip141_rule);
    const auto& ins = *tx.inputs_ptr();
    const auto count = possible_narrow_cast<link_t>(ins.size());
    const auto sigops = tx.signature_operations(bip16, bip141);
    out.positions.emplace(tx.get_hash(false), row);
    out.candidates.push_back(row);
    out.cells.push_back(algorithm::to_column(algorithm::absent));
    out.children.push_back(algorithm::to_column(none));
    out.edges.push_back(algorithm::to_column(
        algorithm::edges_t{ input, count }));

    out.totals.push_back(algorithm::to_column(algorithm::totals_t
    {
        tx.fee(),
        possible_narrow_cast<uint32_t>(tx.virtual_size()),
        possible_narrow_cast<uint32_t>(sigops)
    }));

    for (const auto& in: ins)
    {
        const auto& point = in->point();
        auto parent = none;
        tx_link parent_tx{ in->metadata.parent_tx };
        const auto it = out.positions.find(point.hash());
        if (it != out.positions.end())
            parent = it->second;
        else if (in->metadata.parent_tx == max_uint32)
            parent_tx = to_tx(point.hash());

        if (parent == none && !parent_tx.is_terminal())
        {
            const auto fk = store_.pool.first(parent_tx);
            if (!fk.is_terminal())
                parent = possible_narrow_cast<link_t>(fk.value);
        }

        out.rows.push_back(algorithm::to_column(parent));
        out.siblings.push_back(algorithm::to_column(none));
        out.rivals.push_back(algorithm::to_column(none));

        if (parent != none && parent < rows)
        {
            out.child_bridges.push_back({ parent, input });
        }
        else if (parent != none)
        {
            auto& head = out.children.at(parent - rows);
            out.siblings.back() = head;
            head = algorithm::to_column(input);
        }

        link_t mate{};
        bool conflict{};
        if (!get_pooled_spender(mate, conflict, point, {}, false))
            return error::integrity;

        out.conflicted |= conflict;
        if (mate != none)
        {
            out.rivals.back() = algorithm::to_column(mate);
            out.rival_bridges.push_back({ mate, input });
        }
        else if (const auto at = out.points.find(point);
            at == out.points.end())
        {
            out.points.emplace(point, input);
        }
        else
        {
            auto& ring = out.rivals.at(at->second - inputs);
            const auto vacant = (algorithm::from_column(ring) == none);
            out.rivals.back() = vacant ? algorithm::to_column(at->second) : ring;

            ring = algorithm::to_column(input);
        }

        ++input;
    }

    out.graph.extra =
    {
        out.cells, out.children, out.edges, out.totals,
        out.rows, out.siblings, out.rivals
    };

    out.graph.children = out.child_bridges;
    out.graph.rivals = out.rival_bridges;
    return error::success;
}

TEMPLATE
code CLASS::get_pool_coverage(bool& out, const pool_graph& graph,
    size_t first, uint64_t rate, uint32_t bytes_per_sigop,
    const pool_totals& limit, size_t maximum_outcomes) const NOEXCEPT
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

    // A point of the candidates spent by a confirmed tx cannot be confirmed.
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
            if (algorithm::is_absent(cell))
                return error::success;
        }
    }

    const std::span<const link_t> measured
    {
        std::next(graph.candidates.begin(), first), graph.candidates.end()
    };

    algorithm::initialize(g, graph.candidates);
    auto each = true;
    std::vector<algorithm::component_t> components{};
    for (const auto candidate: measured)
    {
        const std::span<const link_t> self{ &candidate, one };
        const auto seen = [&](const auto& component) NOEXCEPT
        {
            return is_nonzero(algorithm::members(component, self));
        };

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

        const auto best = algorithm::evaluate(masks, component, g);
        const auto bytes = algorithm::chargeable(component, g, bytes_per_sigop);
        each = each && algorithm::covered(best, bytes, rate);
        components.push_back(component);
    }

    // Pooled parent's surplus pays for its child (parent pays for child), in a
    // family within a block, beyond which each component pays its own.
    uint64_t fee{}, bytes{};
    const auto bounded = algorithm::family(fee, bytes, g, measured,
        limit.size, bytes_per_sigop, maximum_outcomes);

    out = bounded ? algorithm::covered(fee, bytes, rate) : each;

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
