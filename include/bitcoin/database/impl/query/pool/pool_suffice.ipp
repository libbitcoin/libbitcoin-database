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
bool CLASS::set_pooled_context(const tx_link& link,
    const context& ctx) NOEXCEPT
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

    auto guard = vtx.get_memory();
    return guard &&
        vtx.flags.put(fk, scalar{ {}, ctx.flags }) &&
        vtx.heights.put(fk, scalar{ {}, ctx.height }) &&
        vtx.medians.put(fk, scalar{ {}, ctx.mtp });
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
