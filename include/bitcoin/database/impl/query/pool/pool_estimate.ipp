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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_ESTIMATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_ESTIMATE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Marks of the rows continue the projection, and any others restart it.
TEMPLATE
code CLASS::get_pool_projection(pool_projection& projection, size_t count,
    const pool_totals& budget, size_t depth, uint64_t cutoff,
    uint32_t bytes_per_sigop, size_t maximum_outcomes) const NOEXCEPT
{
    using algorithm = system::pool::algorithm;
    if (!store_.pool.enabled())
        return error::success;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return error::integrity;

    if (projection.marks.size() != columns.cells.size())
        set_pool_resolution(projection, columns, cutoff, bytes_per_sigop,
            maximum_outcomes);

    pool_links rows{};
    while (!projection.exhausted && projection.blocks.size() < count)
        algorithm::project(rows, projection, columns, budget, depth,
            bytes_per_sigop);

    return error::success;
}

// Rows pooled since the projection was resolved are not of its snapshot.
TEMPLATE
code CLASS::get_pool_continuation(pool_projection& projection, size_t count,
    const pool_totals& budget, size_t depth,
    uint32_t bytes_per_sigop) const NOEXCEPT
{
    using algorithm = system::pool::algorithm;
    if (!store_.pool.enabled())
        return error::success;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return error::integrity;

    if (projection.marks.size() > columns.cells.size())
        return error::integrity;

    projection.marks.resize(columns.cells.size(), algorithm::mark::excluded);

    pool_links rows{};
    while (!projection.exhausted && projection.blocks.size() < count)
        algorithm::project(rows, projection, columns, budget, depth,
            bytes_per_sigop);

    return error::success;
}

// Row arrives at height of its context, and counts while it can confirm or has
// confirmed. Rows of top height are of open interval.
TEMPLATE
code CLASS::get_pool_arrivals(pool_arrivals& out, size_t top, size_t count,
    uint32_t bytes_per_sigop) const NOEXCEPT
{
    using namespace system;
    using algorithm = system::pool::algorithm;
    out.clear();
    out.resize(count);
    if (!store_.pool.enabled() || is_zero(count))
        return error::success;

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return error::integrity;

    for (size_t at{}; at < columns.cells.size(); ++at)
    {
        const auto cell = algorithm::from_column(columns.cells[at]);
        if (!algorithm::is_present(cell) && !algorithm::is_boundary(cell))
            continue;

        const size_t height = algorithm::from_column(columns.heights[at]);
        if (height >= top || (top - height) > count)
            continue;

        const auto totals = algorithm::from_column(columns.totals[at]);
        const auto rate = algorithm::fee_rate(totals, bytes_per_sigop);
        const auto size = algorithm::chargeable(totals, bytes_per_sigop);
        out.at(sub1(top - height)).push_back({ rate, size });
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
