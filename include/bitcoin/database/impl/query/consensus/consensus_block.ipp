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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_BLOCK_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_BLOCK_IPP

#include <algorithm>
#include <atomic>
#include <ranges>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Called by organizer under strict order to determine if block is confirmable.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity_block_confirmable1;

    // bip30 coinbase check (see notes).
    ////code ec{};
    ////if ((ec = spent_duplicates(link, ctx)))
    ////    return ec;

    // Coinbase txs are not populated.
    const auto txs = to_spending_txs(link);
    if (txs.empty())
        return error::success;

    // One point set per tx.
    point_sets sets(txs.size());
    std::atomic<size_t> count{};
    stopper fault{};

    // Get points for each tx and the total count.
    std::transform(parallel, txs.begin(), txs.end(), sets.begin(), 
        [this, &count, &fault](const tx_link& tx) NOEXCEPT
        {
            point_set set{};
            table::transaction::get_set_ref get{ {}, set };
            if (!store_.tx.get(tx, get))
                fault.store(true, relaxed);

            count.fetch_add(set.points.size(), relaxed);
            return set;
        });

    if (fault.load(relaxed))
        return error::integrity_block_confirmable2;

    // Returns database (integrity) or system (consensus) code.
    // Checks double spends strength, populates prevout parent tx/cb/sq links.
    if (const auto ec = get_prevouts(sets, count.load(relaxed), link))
        return ec;

    // Code non-integral (no atomic), so codes must be system::error.
    std::atomic<system::error::transaction_error_t> consensus{};

    // Checks all spends for spendability (strong, unlocked and mature).
    if (std::all_of(parallel, sets.begin(), sets.end(),
        [this, &ctx, &consensus](const point_set& set) NOEXCEPT
        {
            for (const auto& point: set.points)
            {
                if (point.tx.is_terminal()) continue;
                if (const auto ec = spendable(point, set.version, ctx))
                {
                    consensus.store(ec, relaxed);
                    return false;
                }
            }

            return true;
        })) return error::success;

    // Recast previous_output_null as database integrity error.
    const auto result = consensus.load(relaxed);
    if (result == system::error::previous_output_null)
        return error::integrity_spendable;

    return result;
}

// utility
// ----------------------------------------------------------------------------
// All return codes must be system::error::transaction_error_t (see atomic).

TEMPLATE
system::error::transaction_error_t CLASS::spendable(
    const point_set::point& point, uint32_t version,
    const context& ctx) const NOEXCEPT
{
    const auto link = find_strong(point.tx);
    if (link.is_terminal())
        return system::error::unconfirmed_spend;

    // Avoids get_context call when relative locktime is not applicable.
    const auto bip68 = ctx.is_enabled(system::chain::flags::bip68_rule);
    const auto relative = bip68 && transaction::is_relative_locktime_applied(
        point.coinbase, version, point.sequence);

    if (relative || point.coinbase)
    {
        context prevout{};
        if (!get_context(prevout, link))
            return system::error::previous_output_null;

        if (relative &&
            input::is_relative_locked(point.sequence, ctx.height, ctx.mtp,
                prevout.height, prevout.mtp))
            return system::error::relative_time_locked;

        if (point.coinbase &&
            transaction::is_coinbase_immature(prevout.height, ctx.height))
            return system::error::coinbase_maturity;
    }

    return system::error::transaction_success;
}

// ****************************************************************************
// CONSENSUS: To reproduce the behavior of a UTXO accumulator when reorganizing
// a BIP30 exception block, the first instance of the reorganized coinbase tx
// must be set unstrong, despite its block being strong. This creates the odd
// situation where there is a confirmed block with unconfirmed txs. Otherwise
// the txs are spendable, but in the satoshi client their outputs no longer
// exist (de-accumulated). There is discussion about fixing this issue in the
// satoshi client, which would likely result in our behavior without this
// special handling. This is moot given the existence of checkpoints, so
// presently not consensus.
// ****************************************************************************

} // namespace database
} // namespace libbitcoin

#endif
