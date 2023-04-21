/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {

// Block status (surrogate-keyed).
// ----------------------------------------------------------------------------
// Not for use in validatation (2 additional gets).

// protected
TEMPLATE
height_link CLASS::get_height(const header_link& link) const NOEXCEPT
{
    table::header::record_height header{};
    if (!store_.header.get(link, header))
        return {};

    return header.height;
}

TEMPLATE
bool CLASS::is_candidate_block(const header_link& link) const NOEXCEPT
{
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
    const auto fk = to_block(link);
    return !fk.is_terminal() && is_confirmed_block(fk);
}

TEMPLATE
bool CLASS::is_confirmed_input(const input_link& link) const NOEXCEPT
{
    const auto fk = to_input_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_confirmed_output(const output_link& link) const NOEXCEPT
{
    const auto fk = to_output_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_spent_output(const output_link& link) const NOEXCEPT
{
    const auto ins = to_spenders(link);

    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return is_confirmed_input(in);
    });
}

// Confirmation.
// ----------------------------------------------------------------------------
// Strong must be set at current height during organization, unset if fails.

TEMPLATE
bool CLASS::is_mature(const input_link& link, size_t height) const NOEXCEPT
{
    table::input::slab_decomposed_fk input{};
    return store_.input.get(link, input) && (input.is_null() ||
        mature_prevout(input.point_fk, height) == error::success);
}

TEMPLATE
bool CLASS::is_spent(const input_link& link) const NOEXCEPT
{
    table::input::slab_composite_sk input{};
    return store_.input.get(link, input) && !input.is_null() &&
        is_input_spent_prevout(input.key, link);
}

TEMPLATE
bool CLASS::is_strong(const input_link& link) const NOEXCEPT
{
    // to_block checks confirmedness, including de-confirmedness.
    return !to_block(to_input_tx(link)).is_terminal();
}

// protected
TEMPLATE
error::error_t CLASS::locked_prevout(const point_link& link, uint32_t sequence,
    const context& ctx) const NOEXCEPT
{
    if (!script::is_enabled(ctx.flags, system::chain::forks::bip68_rule))
        return error::success;

    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::missing_previous_output;

    // to_block assures confirmation by strong_tx traversal.
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    context prevout_ctx{};
    if (!get_context(prevout_ctx, header_fk))
        return error::integrity;

    if (input::is_locked(sequence, ctx.height, ctx.mtp, prevout_ctx.height,
        prevout_ctx.mtp))
        return error::relative_time_locked;

    return error::success;
}

// protected
TEMPLATE
error::error_t CLASS::mature_prevout(const point_link& link,
    size_t height) const NOEXCEPT
{
    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::integrity;

    // to_block assures confirmation by strong_tx traversal so this must remain
    // prior to is_coinbase in execution order, despite the ack of dependency.
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    if (!is_coinbase(tx_fk))
        return error::success;

    const auto prevout_height = get_height(header_fk);
    if (prevout_height.is_terminal())
        return error::integrity;

    if (!transaction::is_coinbase_mature(prevout_height, height))
        return error::coinbase_maturity;

    return error::success;
}

// protected
TEMPLATE
inline error::error_t CLASS::spendable_prevout(const point_link& link,
    uint32_t sequence, const context& ctx) const NOEXCEPT
{
    ////// input.point->output.hash->output.tx
    ////const auto tx_fk = to_tx(get_point_key(link));
    ////if (tx_fk.is_terminal())
    ////    return error::missing_previous_output;
    ////
    ////// output.tx->output.block
    ////const auto header_fk = to_block(tx_fk);
    ////if (header_fk.is_terminal())
    ////    return error::unconfirmed_spend;
    ////
    ////// output.block->prevout_ctx
    ////context prevout_ctx{};
    ////if (!get_context(prevout_ctx, header_fk))
    ////    return error::integrity;
    ////
    ////// mature_prevout
    ////// output.transaction->output.transaction.coinbase
    ////if (is_coinbase(tx_fk) &&
    ////    !transaction::is_coinbase_mature(prevout_ctx.height, ctx.height))
    ////    return error::coinbase_maturity;
    ////
    ////// locked_prevout
    ////if (script::is_enabled(ctx.flags, system::chain::forks::bip68_rule) &&
    ////    input::is_locked(sequence, ctx.height, ctx.mtp,
    ////        prevout_ctx.height, prevout_ctx.mtp))
    ////        return error::relative_time_locked;

    return spendable_prevout(to_tx(get_point_key(link)), is_coinbase(link),
        sequence, ctx);
}

// protected
TEMPLATE
inline error::error_t CLASS::spendable_prevout(const tx_link& link,
    bool coinbase, uint32_t sequence, const context& ctx) const NOEXCEPT
{
    constexpr auto bip68_rule = system::chain::forks::bip68_rule;

    context out{};
    if (!get_context(out, to_block(link)))
        return error::unconfirmed_spend;

    if (coinbase && !transaction::is_coinbase_mature(out.height, ctx.height))
        return error::coinbase_maturity;

    if (script::is_enabled(ctx.flags, bip68_rule) &&
        input::is_locked(sequence, ctx.height, ctx.mtp, out.height, out.mtp))
        return error::relative_time_locked;

    return error::success;
}

// protected
TEMPLATE
inline bool CLASS::is_input_spent_prevout(const foreign_point& key,
    const input_link& self) const NOEXCEPT
{
    const auto ins = to_spenders(key);
    if (ins.size() < two)
        return false;

    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return (in != self) && is_strong(in);
    });
}

// protected
TEMPLATE
inline bool CLASS::is_tx_spent_prevout(const foreign_point& key,
    const tx_link& self) const NOEXCEPT
{
    auto it = store_.spend.it(key);
    if (it.self().is_terminal())
        return false;

    table::spend::record spend{};
    do
    {
        // Iterated element must be found, otherwise fault.
        if (!store_.spend.get(it.self(), spend))
            return true;

        // Skip self (which should be strong) and require strong for spent.
        if ((spend.tx_fk != self) && !to_block(spend.tx_fk).is_terminal())
            return true;
    }
    while (it.advance());
    return false;
}

TEMPLATE
code CLASS::point_confirmable(const cached_point& point) const NOEXCEPT
{
    code ec{ error::success };

    // height is always needed for coinbase (maturity).
    // sequence value tells which is needed, height or mtp (bip68).
    // but both may apply, so need to allow for two (no flags at prevout).
    const context ctx{ 0, point.height, point.mtp };
    if ((ec = spendable_prevout(point.tx, point.coinbase, point.sequence, ctx)))
        return ec;

    // may only be strong-spent by self (and must be but is not checked).
    if (is_tx_spent_prevout(point.key, point.self))
        return error::confirmed_double_spend;

    return ec;
}

TEMPLATE
bool CLASS::create_cached_points(cached_points& out,
    const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return false;

    table::input::slab_composite_sk_and_sequence_parent input{};
    for (const auto& in: to_non_coinbase_inputs(link))
    {
        if (!store_.input.get(in, input))
            return false;
    
        const auto tx_fk = to_tx(get_point_key(input.point_fk()));
        out.push_back(
        {
            // input (under validation)
            input.key,
            input.parent_fk,
            input.sequence,

            // input->prevout
            tx_fk.value,
            system::possible_narrow_cast<uint32_t>(ctx.height),
            ctx.mtp,
            is_coinbase(tx_fk)
        });
    }

    return true;
}

TEMPLATE
code CLASS::block_confirmable(const input_links& links,
    const context& ctx) const NOEXCEPT
{
    code ec{};
    table::input::slab_composite_sk_and_sequence_parent input{};
    for (const auto& link: links)
    {
        if (!store_.input.get(link, input))
            return error::integrity;

        if (is_tx_spent_prevout(input.key, input.parent_fk))
            return error::confirmed_double_spend;

        if ((ec = spendable_prevout(input.point_fk(), input.sequence, ctx)))
            return ec;
    }

    return error::success;
}

TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity;

    return block_confirmable(to_non_coinbase_inputs(link), ctx);
}

TEMPLATE
bool CLASS::set_strong(const header_link& link) NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, link };

    // ========================================================================
    const auto scope = store_.get_transactor();

    return std::all_of(txs.begin(), txs.end(), [&](const tx_link& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_unstrong(const header_link& link) NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, header_link::terminal };

    // ========================================================================
    const auto scope = store_.get_transactor();

    return std::all_of(txs.begin(), txs.end(), [&](const tx_link& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::initialize(const block& genesis) NOEXCEPT
{
    BC_ASSERT(!is_initialized());
    BC_ASSERT(genesis.transactions_ptr()->size() == one);

    // ========================================================================
    const auto scope = store_.get_transactor();

    const context ctx{};
    if (!set(genesis, ctx))
        return false;

    constexpr auto fees = 0u;
    constexpr auto sigops = 0u;
    const auto link = to_header(genesis.hash());

    return set_strong(header_link{ 0 })
        && set_tx_connected(tx_link{ 0 }, ctx, fees, sigops) // tx valid.
        && set_block_confirmable(link, fees) // rename, block valid step.
        && push_candidate(link)
        && push_confirmed(link);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    const table::height::record confirmed{ {}, link };
    return store_.confirmed.put(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    return store_.candidate.truncate(top);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    return store_.confirmed.truncate(top);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
