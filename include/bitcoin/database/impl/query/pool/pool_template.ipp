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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_TEMPLATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_TEMPLATE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
code CLASS::get_pool_template(tx_links& out, pool_projection& projection,
    const pool_totals& budget, size_t depth, uint64_t cutoff,
    uint32_t bytes_per_sigop, size_t maximum_outcomes) const NOEXCEPT
{
    using algorithm = system::pool::algorithm;
    out.clear();
    projection = {};
    if (!store_.pool.enabled())
        return error::success;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return error::integrity;

    pool_links rows{};
    set_pool_resolution(projection, columns, cutoff, bytes_per_sigop,
        maximum_outcomes);
    algorithm::project(rows, projection, columns, budget, depth,
        bytes_per_sigop);

    out.reserve(rows.size());
    for (const auto row: rows)
    {
        const tx_link tx{ store_.pool.get_key(pool_link{ row }) };
        if (tx.is_terminal())
            return error::integrity;

        out.push_back(tx);
    }

    return error::success;
}

TEMPLATE
code CLASS::get_template_txs(template_txs& out,
    const tx_links& links) const NOEXCEPT
{
    using prevout = table::prevout::slab_get;
    out.clear();
    out.reserve(links.size());
    std::unordered_map<tx_link::integer, size_t> positions{};
    positions.reserve(links.size());
    for (const tx_link link: links)
    {
        table::pool_edge edge{};
        table::pool_total total{};
        const auto fk = store_.pool.first(link);
        if (fk.is_terminal() || !store_.pool.totals.get(fk, total) ||
            !store_.pool.edges.get(fk, edge))
            return error::integrity;

        table::spends::get_refs::parents parents(edge.inputs);
        table::spends::get_refs spent{ {}, parents };
        if (!store_.spends.get(edge.spends_fk, spent))
            return error::integrity;

        template_tx tx{ get_tx_key(link), get_wtxid_key(fk), total.fee,
            total.sigops, {} };
        for (const auto merged: parents)
        {
            const tx_link parent{ prevout::output_tx_fk(merged) };
            const auto it = positions.find(parent.value);
            if (it == positions.end())
                continue;

            if (!system::contains(tx.parents, it->second))
                tx.parents.push_back(it->second);
        }

        positions.emplace(link.value, out.size());
        out.push_back(std::move(tx));
    }

    return error::success;
}

// protected
TEMPLATE
void CLASS::set_pool_resolution(pool_projection& projection,
    const system::pool::algorithm::columns_t& columns, uint64_t cutoff,
    uint32_t bytes_per_sigop, size_t maximum_outcomes) const NOEXCEPT
{
    using algorithm = system::pool::algorithm;
    projection = {};
    projection.marks.resize(columns.cells.size());
    algorithm::resolve(projection.marks, columns, maximum_outcomes, cutoff,
        bytes_per_sigop);
}

} // namespace database
} // namespace libbitcoin

#endif
