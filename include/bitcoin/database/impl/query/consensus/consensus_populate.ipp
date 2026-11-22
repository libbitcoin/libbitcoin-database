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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_POPULATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_POPULATE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Input population not expected to be called publicly, tx population (chain
// true) expected to be called (tx pool validation), and block population called
// for full block validation/confirmation (chain false).

// populate_with_metadata
// ----------------------------------------------------------------------------
// Used when performing confirmation, either directly (chain true) or caching
// metadata (chain false) in validation stage for later confirmation.

TEMPLATE
bool CLASS::populate_with_metadata(const block& block,
    bool chain) const NOEXCEPT
{
    const auto& txs = block.transactions_ptr();
    if (txs->empty())
        return false;

    return std::all_of(std::next(txs->begin()), txs->end(),
        [this, chain](const auto& tx) NOEXCEPT
        {
            return this->populate_with_metadata(*tx, chain);
        });
}

TEMPLATE
bool CLASS::populate_with_metadata(const transaction& tx, bool chain,
    bool pool) const NOEXCEPT
{
    // This override makes the public method safe for coinbase calling.
    return tx.is_coinbase() || populate_with_metadata_(tx, chain, pool);
}

// protected
TEMPLATE
bool CLASS::populate_with_metadata_(const transaction& tx, bool chain,
    bool pool) const NOEXCEPT
{
    BC_ASSERT(!tx.is_coinbase());

    const auto& ins = tx.inputs_ptr();
    return std::all_of(ins->begin(), ins->end(),
        [this, chain, pool](const auto& in) NOEXCEPT
        {
            return this->populate_with_metadata(*in, chain, pool);
        });
}

TEMPLATE
bool CLASS::populate_with_metadata(const input& input, bool chain,
    bool pool) const NOEXCEPT
{
    // Null point would return nullptr and be interpreted as missing.
    BC_ASSERT(!input.point().is_null());

    // If read via store for store confirmation, input.metadata.point_link is
    // set earlier in get_input(), so this assertion does not hold.
    ////BC_ASSERT(input.metadata.point_link == max_uint32);

    if (input.prevout)
        return true;

    const auto tx = to_tx(input.point().hash());
    const auto index = input.point().index();

    // Node and chain confirmation.
    input.prevout = get_output(tx, index);
    input.metadata.coinbase = is_coinbase(tx);

    // Node only (cheap so always include).
    input.metadata.parent_tx = tx;

    // Used by node for unconfirmed tx validation (expensive).
    // Three consensus-relevant heights: height of presumed next block (pool),
    // height of block containing previous output's transaction, and height of
    // block containing a previous spender of previous output. median-time-past
    // is of the prevout.
    if (chain)
    {
        auto& metadata = input.metadata;

        context ctx{};
        if (get_context(ctx, find_strong(metadata.parent_tx)))
        {
            // Confirmed previous output found at height (and with mtp).
            metadata.prevout_height = ctx.height;
            metadata.median_time_past = ctx.mtp;
        }
        else
        {
            // Confirmed previous output not found.
            metadata.prevout_height = max_uint32;
            metadata.median_time_past = max_uint32;
        }

        // Genesis coinbase is unspendable, so no confirmed spender is at zero,
        // which therefore distinguishes unconfirmed spender.
        if (pool && !is_spent(input.point()))
        {
            // Any spender (including duplicate tx) not found.
            metadata.spender_height = max_uint32;
        }
        else if (const auto height = find_strong_spender_height(input.point());
            !height.is_terminal())
        {
            // Confirmed spender found at height.
            metadata.spender_height = system::possible_narrow_cast<uint32_t>(
                height.value);
        }
        else
        {
            // Confirmed spender not found.
            metadata.spender_height = pool ? 0_u32 : max_uint32;
        }
    }

    return !is_null(input.prevout);
}

// populate_pooled
// ----------------------------------------------------------------------------
// Used in validation of block containing a tx validated in pool, caching pool's
// metadata for later confirmation. Block-internal spend is populated by block,
// so its metadata remains that of internal spend.

TEMPLATE
code CLASS::populate_pooled(pooled_tx& out, const transaction& tx,
    const tx_link& link, const context& ctx) const NOEXCEPT
{
    const auto& ins = *tx.inputs_ptr();
    out.prevouts.resize(ins.size());
    if (const auto ec = get_pooled(out, link, ctx))
        return ec;

    auto prevout = out.prevouts.cbegin();
    for (const auto& in: ins)
    {
        const auto& pooled = *prevout++;
        if (in->prevout)
            continue;

        in->metadata.parent_tx = pooled.parent;
        in->metadata.coinbase = pooled.coinbase;
    }

    return error::success;
}

// populate_without_metadata
// ----------------------------------------------------------------------------
// Used when not performing confirmation. Also implies validation is not being
// performed, so used for populating prevouts to compute client filters in
// validation stage. Not used in consensus but kept here for close similarity.

TEMPLATE
bool CLASS::populate_without_metadata(const block& block) const NOEXCEPT
{
    const auto& txs = block.transactions_ptr();
    if (txs->empty())
        return false;

    return std::all_of(std::next(txs->begin()), txs->end(),
        [this](const auto& tx) NOEXCEPT
        {
            return this->populate_without_metadata(*tx);
        });
}

TEMPLATE
bool CLASS::populate_without_metadata(const transaction& tx) const NOEXCEPT
{
    BC_ASSERT(!tx.is_coinbase());

    const auto& ins = tx.inputs_ptr();
    return std::all_of(ins->begin(), ins->end(),
        [this](const auto& in) NOEXCEPT
        {
            return this->populate_without_metadata(*in);
        });
}

TEMPLATE
bool CLASS::populate_without_metadata(const input& input) const NOEXCEPT
{
    // Null point would return nullptr and be interpreted as missing.
    BC_ASSERT(!input.point().is_null());

    if (input.prevout)
        return true;

    input.prevout = get_output(to_output(input.point()));
    return !is_null(input.prevout);
}

} // namespace database
} // namespace libbitcoin

#endif
