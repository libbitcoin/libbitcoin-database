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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_SUFFICE_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_SUFFICE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// The context height of a row that is not validated, above every height.
constexpr uint32_t unvalidated_height = max_uint32;

TEMPLATE
code CLASS::get_pool_revalidation(tx_links& out,
    const context& ctx) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    out.clear();
    if (!store_.pool.enabled())
        return error::success;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return error::integrity;

    const auto rows = possible_narrow_cast<link_t>(columns.cells.size());
    for (link_t row{}; row < rows; ++row)
    {
        const auto cell = algorithm::from_column(columns.cells[row]);
        if (!algorithm::is_absent(cell) ||
            is_zero(bit_and(cell, algorithm::insufficient)))
            continue;

        const auto flags = algorithm::from_column(columns.flags[row]);
        if (bit_and(flags, ctx.flags) == ctx.flags ||
            algorithm::from_column(columns.heights[row]) > ctx.height ||
            algorithm::from_column(columns.medians[row]) > ctx.mtp)
            continue;

        const auto tx = to_pool_tx(pool_link{ row });
        if (!tx.is_terminal())
            out.push_back(tx);
    }

    return error::success;
}

TEMPLATE
code CLASS::get_pool_unvalidated(tx_links& out) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    using link_t = algorithm::link_t;
    out.clear();
    if (!store_.pool.enabled())
        return error::success;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return error::integrity;

    const auto rows = possible_narrow_cast<link_t>(columns.cells.size());
    for (link_t row{}; row < rows; ++row)
    {
        const auto cell = algorithm::from_column(columns.cells[row]);
        if (!algorithm::is_absent(cell) ||
            is_zero(bit_and(cell, algorithm::insufficient)) ||
            algorithm::from_column(columns.heights[row]) != unvalidated_height)
            continue;

        const auto tx = to_pool_tx(pool_link{ row });
        if (!tx.is_terminal())
            out.push_back(tx);
    }

    return error::success;
}

TEMPLATE
bool CLASS::set_pooled_context(const tx_link& link, const context& ctx,
    size_t sigops) NOEXCEPT
{
    return set_pooled_contexts({ link.value }, { sigops }, ctx);
}

// The sigops of a tx are counted under the flags of its context.
TEMPLATE
bool CLASS::set_pooled_contexts(const tx_links& links,
    const std::vector<size_t>& sigops, const context& ctx) NOEXCEPT
{
    if (!store_.pool.enabled())
        return true;

    if (links.size() != sigops.size())
        return false;

    std::unordered_map<tx_link::integer, size_t> counts{};
    counts.reserve(links.size());
    for (size_t index{}; index < links.size(); ++index)
        counts.emplace(links.at(index), sigops.at(index));

    // ========================================================================
    const auto scope = get_transactor();

    tx_links purged{};
    return set_pool_purged(purged, links, true) && set_pool_repooled(purged,
        [&](context& stored, size_t& count, const tx_link& link) NOEXCEPT
        {
            const auto it = counts.find(link.value);
            if (it == counts.end())
                return;

            stored = ctx;
            count = it->second;
        });
    // ========================================================================
}

// Tx spending an output of block (coinbase included) may depend on its
// confirmation, so not sufficient under any context until revalidated. Its
// flags are kept, as its sigops are counted under them.
TEMPLATE
bool CLASS::set_pool_unvalidated(size_t& out,
    const header_link& link) NOEXCEPT
{
    out = zero;
    if (!store_.pool.enabled())
        return true;

    tx_links spenders{};
    std::unordered_set<tx_link::integer> unique{};
    for (const auto fk: to_transactions(link))
    {
        const tx_link tx{ fk };
        const auto outputs = to_outputs(tx).size();
        for (uint32_t index{}; index < outputs; ++index)
        {
            for (const auto& in: to_spenders(tx, index))
            {
                const auto spender = to_input_tx(in);
                if (!store_.pool.first(spender).is_terminal() &&
                    unique.insert(spender.value).second)
                    spenders.push_back(spender.value);
            }
        }
    }

    out = spenders.size();

    // ========================================================================
    const auto scope = get_transactor();

    tx_links purged{};
    return set_pool_purged(purged, spenders, true) && set_pool_repooled(purged,
        [&](context& stored, size_t&, const tx_link& spender) NOEXCEPT
        {
            if (unique.contains(spender.value))
                stored.height = unvalidated_height;
        }) && store_.set_pool_top(to_parent(link));
    // ========================================================================
}

TEMPLATE
bool CLASS::set_pool_context(size_t& out, const context& ctx) NOEXCEPT
{
    using algorithm = system::pool::algorithm;
    out = zero;
    if (!store_.pool.enabled())
        return true;

    // ========================================================================
    const auto scope = get_transactor();

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    system::pool::suffice(out, columns, { ctx.flags, ctx.height, ctx.mtp },
        system::chain::flags::bip113_rule);

    system::pool::settle(columns);
    system::pool::label(columns);
    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
