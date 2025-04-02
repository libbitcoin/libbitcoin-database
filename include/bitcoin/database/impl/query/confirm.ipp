/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {

// Block status (mostly surrogate-keyed).
// ----------------------------------------------------------------------------
// Not for use in validatation (2 additional gets) or confirmation (height).

// protected/get_confirmed_balance(address)
TEMPLATE
bool CLASS::is_confirmed_unspent(const output_link& link) const NOEXCEPT
{
    return is_confirmed_output(link) && !is_spent_output(link);
}

TEMPLATE
bool CLASS::is_candidate_header(const header_link& link) const NOEXCEPT
{
    // The header is candidate (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record candidate{};
    return store_.candidate.get(height, candidate) &&
        (candidate.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_block(const header_link& link) const NOEXCEPT
{
    // The block is confirmed (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record confirmed{};
    return store_.confirmed.get(height, confirmed) &&
        (confirmed.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_tx(const tx_link& link) const NOEXCEPT
{
    // The tx is strong *and* its block is confirmed (by height).
    const auto fk = to_block(link);
    return !fk.is_terminal() && is_confirmed_block(fk);
}

TEMPLATE
bool CLASS::is_confirmed_input(const point_link& link) const NOEXCEPT
{
    // The spend.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_spending_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_confirmed_output(const output_link& link) const NOEXCEPT
{
    // The output.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_output_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_spent_output(const output_link& link) const NOEXCEPT
{
    // The spender is strong *and* its block is confirmed (by height).
    const auto ins = to_spenders(link);
    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return is_confirmed_input(in);
    });
}

// Block confirmed by height is not used for confirmation (just strong tx).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_strong_tx(const tx_link& link) const NOEXCEPT
{
    table::strong_tx::record strong{};
    return store_.strong_tx.find(link, strong) && strong.positive();
}

TEMPLATE
bool CLASS::is_strong_block(const header_link& link) const NOEXCEPT
{
    return is_strong_tx(to_coinbase(link));
}

////// unused
////TEMPLATE
////bool CLASS::is_strong_spend(const point_link& link) const NOEXCEPT
////{
////    return is_strong_tx(to_spending_tx(link));
////}
////
////// unused
////TEMPLATE
////bool CLASS::is_mature(const point_link& link, size_t height) const NOEXCEPT
////{
////    const auto key = get_point_hash(link);
////    if (key == system::null_hash)
////        return true;
////
////    return !mature_prevout(key, height);
////}
////
////// protected (only for is_mature/unused)
////TEMPLATE
////code CLASS::mature_prevout(const hash_digest& hash,
////    size_t height) const NOEXCEPT
////{
////    // Search for prevout tx and get its link.
////    const auto tx_fk = to_tx(hash);
////    const auto header_fk = to_block(tx_fk);
////    if (header_fk.is_terminal())
////        return error::unconfirmed_spend;
////
////    // Must also be strong (above).
////    if (!is_coinbase(tx_fk))
////        return error::success;
////
////    const auto prevout_height = get_height(header_fk);
////    if (prevout_height.is_terminal())
////        return error::integrity;
////
////    if (!transaction::is_coinbase_mature(prevout_height, height))
////        return error::coinbase_maturity;
////
////    return error::success;
////}

////TEMPLATE
////bool CLASS::is_locked(const point_link& link, uint32_t sequence,
////    const context& ctx) const NOEXCEPT
////{
////    return !locked_prevout(link, sequence, ctx);
////}
////
////TEMPLATE
////code CLASS::locked_prevout(const point_link& link, uint32_t sequence,
////    const context& ctx) const NOEXCEPT
////{
////    if (!ctx.is_enabled(system::chain::flags::bip68_rule))
////        return error::success;
////
////    // Get hash from point, search for prevout tx and get its link.
////    table::transaction::get_version tx{};
////    const auto tx_fk = store_.tx.find(get_point_hash(link), tx);
////    if (tx_fk.is_terminal())
////        return error::missing_previous_output;
////
////    if (tx.version < system::chain::relative_locktime_min_version)
////        return error::success;
////
////    // to_block assures confirmation by strong_tx traversal.
////    const auto header_fk = to_block(tx_fk);
////    if (header_fk.is_terminal())
////        return error::unconfirmed_spend;
////
////    context prevout_ctx{};
////    if (!get_context(prevout_ctx, header_fk))
////        return error::integrity;
////
////    if (input::is_relative_locked(sequence, ctx.height, ctx.mtp,
////        prevout_ctx.height, prevout_ctx.mtp))
////        return error::relative_time_locked;
////
////    return error::success;
////}

} // namespace database
} // namespace libbitcoin

#endif
