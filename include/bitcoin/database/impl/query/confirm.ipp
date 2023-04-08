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
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

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
bool CLASS::is_strong(const input_link& link) const NOEXCEPT
{
    return !to_block(to_input_tx(link)).is_terminal();
}

TEMPLATE
bool CLASS::is_spent(const input_link& link) const NOEXCEPT
{
    table::input::slab_composite_sk input{};
    return store_.input.get(link, input) && !input.is_null() &&
        is_spent_prevout(input.key, link);
}

// protected
TEMPLATE
bool CLASS::is_spent_prevout(const table::input::search_key& key,
    const input_link& self) const NOEXCEPT
{
    BC_ASSERT(key != table::input::null_point());

    // Input is one spender, must be second for output to have been spent.
    const auto ins = to_spenders(key);
    if (ins.size() < two)
        return false;

    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return (in != self) && is_strong(in);
    });
}

TEMPLATE
bool CLASS::is_mature(const input_link& link, size_t height) const NOEXCEPT
{
    table::input::slab_decomposed_fk input{};
    return store_.input.get(link, input) && (input.is_null() ||
        mature_prevout(input.point_fk, height) ==
            system::error::transaction_success);
}

////// protected
////TEMPLATE
////bool CLASS::is_unspent_coinbase(const header_link& link) const NOEXCEPT
////{
////    return !is_spent_tx(to_coinbase(link));
////}
////
////// protected
////TEMPLATE
////bool CLASS::is_spent_tx(const tx_link& link) const NOEXCEPT
////{
////    const auto outs = to_tx_outputs(link);
////    return std::all_of(outs.begin(), outs.end(), [&](const auto& out) NOEXCEPT
////    {
////        return is_spent_output(out);
////    });
////}

// protected
TEMPLATE
code CLASS::locked_input(const input_link& link, uint32_t sequence,
    size_t height, uint32_t mtp) const NOEXCEPT
{
    // to_block traverses (assures) confirmation.

    context ctx{};
    if (!get_context(ctx, to_block(to_prevout_tx(link))))
        return database::error::integrity;

    if (input::is_locked(sequence, height, mtp, ctx.height, ctx.mtp))
        return system::error::relative_time_locked;

    return system::error::transaction_success;
}

// protected
TEMPLATE
code CLASS::mature_prevout(const point_link& link,
    size_t height) const NOEXCEPT
{
    const auto spent_fk = to_tx(store_.point.get_key(link));
    if (spent_fk.is_terminal())
        return database::error::integrity;

    // to_block traverses (assures) confirmation.
    const auto header_fk = to_block(spent_fk);
    if (header_fk.is_terminal())
        return system::error::unconfirmed_spend;
    if (!is_coinbase(spent_fk))
        return system::error::transaction_success;

    const auto prevout_height = get_height(header_fk);
    if (prevout_height.is_terminal())
        return database::error::integrity;
    if (!transaction::is_coinbase_mature(prevout_height, height))
        return system::error::coinbase_maturity;

    return system::error::transaction_success;
}

TEMPLATE
code CLASS::chain_confirmable(const header_link& link,
    bool enable_locktime, bool) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return database::error::integrity;

    ////// A problem here is that a tx fully spent can be reused.
    ////// But in that case it must become unspent, since only one copy is stored.
    ////// This presents conflict with prior instance. Pruning is also problematic.
    ////// This code would reject an unspent duplicate, and allow spent duplicate,
    ////// but the new tx over the spent previous tx would indicate spent. It
    ////// Could be explicitly unspent using the strong_tx overwrite of terminal,
    ////// but that makes both appear unspent (which is probably ok). Pruning
    ////// would only allow prune of what appeared spent, which would be ok either
    ////// way.
    ////if (disallow_duplicates && is_unspent_coinbase(link))
    ////    return system::error::unspent_coinbase_collision;

    const auto ins = to_block_inputs(link);
    if (ins.empty())
        return system::error::missing_previous_output;

    code ec;
    table::input::slab_composite_sk_and_sequence input{};
    for (const auto& in: ins)
    {
        if (!store_.input.get(in, input))
            return database::error::integrity;  
        if (!input.is_null())
            continue;
        if (is_spent_prevout(input.key, in))
            return system::error::confirmed_double_spend;
        if (enable_locktime &&
            ((ec = locked_input(in, input.sequence, ctx.height, ctx.mtp))))
            return ec;
        if ((ec = mature_prevout(input.point_fk(), ctx.height)))
            return ec;
    }

    return system::error::block_success;
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
