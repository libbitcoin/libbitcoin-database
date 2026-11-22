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

// The sigops of a tx are counted under the flags of its context.
TEMPLATE
bool CLASS::set_pooled_context(const tx_link& link, const context& ctx,
    size_t sigops) NOEXCEPT
{
    using scalar = table::pool_scalar;
    if (!store_.pool.enabled())
        return true;

    // ========================================================================
    const auto scope = get_transactor();

    auto& vtx = store_.pool;
    const auto fk = vtx.first(link);
    if (fk.is_terminal())
        return false;

    table::pool_total total{};
    auto guard = vtx.get_memory();
    if (!guard || !vtx.totals.get(fk, total))
        return false;

    total.sigops = system::possible_narrow_cast<uint32_t>(sigops);
    return vtx.totals.put(fk, total) &&
        vtx.flags.put(fk, scalar{ {}, ctx.flags }) &&
        vtx.heights.put(fk, scalar{ {}, ctx.height }) &&
        vtx.medians.put(fk, scalar{ {}, ctx.mtp });
    // ========================================================================
}

// A tx spending an output of the block (coinbase included) may depend on its
// confirmation, so it is not sufficient under any context until revalidated.
// Its flags are kept, as its sigops are counted under them.
TEMPLATE
bool CLASS::set_pool_unvalidated(size_t& out,
    const header_link& link) NOEXCEPT
{
    using scalar = table::pool_scalar;
    out = zero;
    if (!store_.pool.enabled())
        return true;

    // ========================================================================
    const auto scope = get_transactor();

    decltype(store_.pool.get_memory()) guard{};
    for (const auto fk: to_transactions(link))
    {
        const tx_link tx{ fk };
        const auto outputs = to_outputs(tx).size();
        for (uint32_t index{}; index < outputs; ++index)
        {
            for (const auto& in: to_spenders(tx, index))
            {
                const auto row = store_.pool.first(to_input_tx(in));
                if (row.is_terminal())
                    continue;

                if (!guard)
                    guard = store_.pool.get_memory();

                if (!guard || !store_.pool.heights.put(row,
                    scalar{ {}, unvalidated_height }))
                    return false;

                ++out;
            }
        }
    }

    return true;
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
