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

// A tx pooled more than once is taken at its first row, which precedes the
// children of each of its rows.
TEMPLATE
code CLASS::get_pool_template(tx_links& out, pool_projection& projection,
    const pool_totals& budget, size_t depth, uint32_t bytes_per_sigop,
    size_t maximum_outcomes) const NOEXCEPT
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
    projection.marks.resize(columns.cells.size());
    algorithm::resolve(projection.marks, columns, maximum_outcomes);
    algorithm::project(rows, projection, columns, budget, depth,
        bytes_per_sigop);

    std::unordered_set<tx_link::integer> taken{};
    out.reserve(rows.size());
    for (const auto row: rows)
    {
        const tx_link tx{ store_.pool.get_key(pool_link{ row }) };
        if (tx.is_terminal())
            return error::integrity;

        if (taken.insert(tx.value).second)
            out.push_back(tx);
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
