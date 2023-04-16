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

    // Spender count is low, so no parallel here.
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
        is_spent_prevout(input.key, link);
}

TEMPLATE
bool CLASS::is_strong(const input_link& link) const NOEXCEPT
{
    return !to_block(to_input_tx(link)).is_terminal();
}

// protected
TEMPLATE
bool CLASS::is_spent_prevout(const table::input::search_key& key,
    const input_link& self) const NOEXCEPT
{
    // For performance avoid calling with null point (always false).
    // Input is one spender, must be second for output to have been spent.
    const auto ins = to_spenders(key);
    if (ins.size() < two)
        return false;

    // Spender count is low, so no parallel here.
    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return (in != self) && is_strong(in);
    });
}

// protected
TEMPLATE
error::error_t CLASS::locked_prevout(const point_link& link, uint32_t sequence,
    const database::context& put) const NOEXCEPT
{
    // bip68 activates at 419328 and is not applicable to coinbase.
    using namespace system::chain;
    if (!system::chain::script::is_enabled(put.flags, forks::bip68_rule))
        return error::success;

    // REDUNDANT.
    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::integrity;

    // REDUNDANT.
    // to_block assures confirmation by strong_tx traversal.
    // store_.strong_tx.get(store_.strong_tx.first(tx_fk), strong);
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    // REDUNDANT.
    // TODO: fold mature_prevout::get_height with locked_prevout::get_context.
    // store_.header.get(link, header);
    context ctx{};
    if (!get_context(ctx, header_fk))
        return error::integrity;

    if (input::is_locked(sequence, put.height, put.mtp, ctx.height, ctx.mtp))
        return error::relative_time_locked;

    return error::success;
}

// protected
TEMPLATE
error::error_t CLASS::mature_prevout(const point_link& link,
    size_t height) const NOEXCEPT
{
    // REDUNDANT.
    // Get hash from point, search for prevout tx and get its link.
    // store_.point.get_key(link); store_.tx.first(key);
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::integrity;

    // REDUNDANT.
    // to_block assures confirmation by strong_tx traversal so this must remain
    // prior to is_coinbase in execution order, despite the ack of dependency.
    // store_.strong_tx.get(store_.strong_tx.first(tx_fk), strong);
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    // store_.tx.get(link, tx) && tx.coinbase;
    if (!is_coinbase(tx_fk))
        return error::success;

    // REDUNDANT.
    // TODO: fold mature_prevout::get_height with locked_prevout::get_context.
    // store_.header.get(link, header);
    const auto prevout_height = get_height(header_fk);
    if (prevout_height.is_terminal())
        return error::integrity;

    if (!transaction::is_coinbase_mature(prevout_height, height))
        return error::coinbase_maturity;

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
code CLASS::block_confirmable(const input_links& links,
    const context& ctx) const NOEXCEPT
{
    // TODO: find a way to test the cost of block_confirmable with cached
    // input_fp and prevout tx_fk values (after folding locked into mature).
    // Maybe populate cached std::vector<std::pair<input::search_key, tx_link>>
    // and perform a warm run on it.

    // This is too expensive, spans all 2.5 billion inputs.
    // We have the necessary info when the tx comes over the wire (hash/index).
    // It gets lost if the block tx is serialized and discarded before confirm.
    // Same holds for prevouts (cache into a circular buffer while validating).
    // We could cache the input foreign points for each block as it is archived.
    // This set and context is all that is required for confirmation. A fp is 7
    // bytes, so for a 10,000 input block, that would be 70,000 bytes and can
    // be stored as a simple vector of arrays (contiguous). These aren't input
    // primary keys, these are input search keys. They are used to search input
    // for spend conflicts (is_spent_prevout).The point_fk of the fp obtains
    // the prevout hash via direct link and can search tx for maturity. But
    // the tx_fk from the prevout may be cached as well, allowing maturity.
    // This requires no association with the input or output, just that the
    // prevout tx is mature. tx_fx is 4 bytes and can be cached as well. So
    // with all input fps and prevout tx fks the confirmation should be fast.
    // There would be only block/tx level events and one is_spent_prevout.

    code ec{};
    table::input::slab_composite_sk_and_sequence input{};

    for (const auto& in: links)
    {
        // This is cheap.
        if (!store_.input.get(in, input))
            return error::integrity;

        // This is expensive.
        // This is an input table search and tx, has no output context.
        // Spent by more than this spender, where that input is confirmed?
        if (is_spent_prevout(input.key, in))
            return error::confirmed_double_spend;

        // This is expensive.
        // This is tx only, not output context.
        // TODO: create is_spendable_prevout() to compliment is_spent_prevout().
        // TODO: combine maturity/locked into query for height|mtp|na.
        // TODO: first walk to confirmed-ness, then if prevout cb get height.
        // TOOD: and if bip68 get height (if not got) or mtp, call chain fns.

        // Strong prevout, height|mtp (based on sequence).
        if ((ec = locked_prevout(input.point_fk(), input.sequence, ctx)))
            return ec;

        // Strong prevout, height (if coinbase).
        if ((ec = mature_prevout(input.point_fk(), ctx.height)))
            return ec;
    }

    return error::success;
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

    return std_all_of(bc::seq, txs.begin(), txs.end(),
        [&](const tx_link& fk) NOEXCEPT
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

    return std_all_of(bc::seq, txs.begin(), txs.end(),
        [&](const tx_link& fk) NOEXCEPT
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
