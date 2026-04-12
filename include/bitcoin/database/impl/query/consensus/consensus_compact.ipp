/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_COMPACT_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_COMPACT_IPP

#include <iterator>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Compact blocks.
/// TODO: apply these to compact block confirmation, as the block will
/// TODO: associate existing txs, making it impossible to rely on the
/// TODO: duplicates table. The full query approach must be used instead.
// ----------------------------------------------------------------------------
// protected

TEMPLATE
bool CLASS::get_double_spenders(tx_links& out, const point& point,
    const point_link& self) const NOEXCEPT
{
    // This is most of the expense of compact block confirmation.
    // It is not mitigated by the point table filter, since self always exists.

    point_links points{};
    for (auto it = store_.point.it(point); it; ++it)
        if (*it != self)
            points.push_back(*it);

    for (auto point: points)
    {
        table::ins::get_parent get{};
        if (!store_.ins.get(point, get))
            return false;

        out.push_back(get.parent_fk);
    }

    return true;
}

TEMPLATE
bool CLASS::get_double_spenders(tx_links& out,
    const block& block) const NOEXCEPT
{
    // Empty or coinbase only implies no spends.
    const auto& txs = *block.transactions_ptr();
    if (txs.size() <= one)
        return true;

    for (auto tx = std::next(txs.cbegin()); tx != txs.cend(); ++tx)
        for (const auto& in: *(*tx)->inputs_ptr())
            if (!get_double_spenders(out, in->point(), in->metadata.point_link))
                return false;

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
