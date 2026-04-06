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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_POPULATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_POPULATE_IPP

#include <iterator>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Input population is not expected to be called publicly, tx population (chain
// true) is expected to be called (tx pool validation), and block population is
// called for full block validation/confirmation (chain false).

// populate_with_metadata
// ----------------------------------------------------------------------------
// These are used when performing confirmation, either directly (chain true) or
// caching metadata (chain false) in validation stage for later confirmation.

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
bool CLASS::populate_with_metadata(const transaction& tx,
    bool chain) const NOEXCEPT
{
    // This override makes the public method safe for coinbase calling.
    return tx.is_coinbase() || populate_with_metadata_(tx, chain);
}

// protected
TEMPLATE
bool CLASS::populate_with_metadata_(const transaction& tx,
    bool chain) const NOEXCEPT
{
    BC_ASSERT(!tx.is_coinbase());

    const auto& ins = tx.inputs_ptr();
    return std::all_of(ins->begin(), ins->end(),
        [this, chain](const auto& in) NOEXCEPT
        {
            return this->populate_with_metadata(*in, chain);
        });
}

TEMPLATE
bool CLASS::populate_with_metadata(const input& input,
    bool chain) const NOEXCEPT
{
    // Null point would return nullptr and be interpreted as missing.
    BC_ASSERT(!input.point().is_null());

    // input.metadata.point_link must be defaulted to max_uint32.
    BC_ASSERT(input.metadata.point_link == max_uint32);

    if (input.prevout)
        return true;

    const auto tx = to_tx(input.point().hash());
    const auto index = input.point().index();

    // Node and chain confirmation.
    input.prevout = get_output(tx, index);
    input.metadata.coinbase = is_coinbase(tx);

    // Node only (cheap so always include).
    input.metadata.parent_tx = tx;

    // This is used by node for unconfirmed tx validation (expensive).
    // There are three consensus-relevant heights. These are the height of the
    // presumed next block (pool), the height of the block containing previous
    // output's transaction, and the height of a block containing a previous
    // spender of the previous output. median-time-past is of the prevout.
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

        if (const auto height = find_strong_spender_height(input.point());
            !height.is_terminal())
        {
            // Confirmed spender found at height.
            metadata.spender_height = system::possible_narrow_cast<uint32_t>(
                height.value);
        }
        else
        {
            // Confirmed spender not found.
            metadata.spender_height = max_uint32;
        }
    }

    // If read via the store for store confirmation, then...
    // input.metadata.point_link must be set earlier in get_input().
    ////BC_ASSERT(input.metadata.point_link != max_uint32);
    return !is_null(input.prevout);
}

// populate_without_metadata
// ----------------------------------------------------------------------------
// These are used when not performing confirmation. This also implies that
// validation is not being performed, so is used for populating prevouts for
// the purpose of computing client filters in the validation stage. So these
// are not used for in consensus but are kept here for close similarity.

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
