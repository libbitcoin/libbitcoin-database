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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_PREVOUTS_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_PREVOUTS_IPP

#include <iterator>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// Prevouts caching.
// Prevouts are cached during validation and read during confirmation.
// The table is purged once per process execution when node is coalesced.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_prevouts(point_sets& sets, size_t points,
    const header_link& link) const NOEXCEPT
{
    // Don't hit prevout table for empty block.
    if (sets.empty())
        return error::success;

    // An empty block will fail here as no prevout element is populated.
    table::prevout::slab_get cache{};
    cache.spends.resize(points);
    const auto prevout = to_prevout(link);

    // Transactor required for prevout read because of pruning.
    // ========================================================================
    {
        const auto scope = store_.get_transactor();

        if (!store_.prevout.at(prevout, cache))
            return error::integrity_get_prevouts;

    }
    // ========================================================================

    // Is any duplicated point in the block confirmed (generally empty).
    for (const auto& spender: cache.conflicts)
        if (is_strong_tx(spender))
            return system::error::confirmed_double_spend;

    // Augment spend.points with metadata.
    auto it = cache.spends.begin();
    for (auto& set: sets)
    {
        for (auto& point: set.points)
        {
            const auto& pair = *it++;
            point.tx = table::prevout::slab_get::output_tx_fk(pair.first);
            point.coinbase = table::prevout::slab_get::coinbase(pair.first);
            point.sequence = pair.second;
        }
    }

    return error::success;
}

TEMPLATE
bool CLASS::set_prevouts(const header_link& link, const block& block) NOEXCEPT
{
    // Empty or coinbase only implies no spends.
    if (block.transactions() <= one)
        return true;

    // Body size check avoids a header hit when no duplicates (common).
    tx_links doubles{};
    if (!is_zero(store_.duplicate.body_size()) && !get_doubles(doubles, block))
        return false;

    const auto prevout = to_prevout(link);

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::prevout::slab_put_ref prevouts{ {}, doubles, block };
    return store_.prevout.put(prevout, prevouts);
    // ========================================================================
}

// utility
// ----------------------------------------------------------------------------
// protected

TEMPLATE
bool CLASS::get_doubles(tx_links& out, const point& point) const NOEXCEPT
{
    if (!store_.duplicate.exists(point))
        return true;

    // Get the [tx.hash:index] of each spender of the point (index unused).
    const auto spenders = get_spenders(point);
    bool found{};

    // Inpoints are deduped by the std::set.
    for (const auto& spender: spenders)
    {
        // Exhaustive enumeration of all txs with spender hash.
        for (auto it = store_.tx.it(spender.hash()); it != it.end(); ++it)
        {
            found = true;
            out.push_back(*it);
        }
    }

    return found;
}

TEMPLATE
bool CLASS::get_doubles(tx_links& out, const block& block) const NOEXCEPT
{
    // Empty or coinbase-only implies no spends.
    const auto& txs = *block.transactions_ptr();
    if (txs.size() <= one)
        return true;

    for (auto tx = std::next(txs.cbegin()); tx != txs.cend(); ++tx)
        for (const auto& in: *(*tx)->inputs_ptr())
            if (!get_doubles(out, in->point()))
                return false;

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
