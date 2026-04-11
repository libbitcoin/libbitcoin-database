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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_HISTORY_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_HISTORY_IPP

#include <atomic>
#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address history
// ----------------------------------------------------------------------------
// Canonically-sorted/deduped address history.
// root txs (height:zero) sorted before transitive (height:max) txs.
// tied-height transactions sorted by base16 txid (not converted).
// All confirmed txs are root, unconfirmed may or may not not be root.

// server/electrum
TEMPLATE
code CLASS::get_unconfirmed_history(const stopper& , histories& ,
    const hash_digest& , bool ) const NOEXCEPT
{
    return {};
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_history(const stopper& , histories& ,
    const hash_digest& , bool ) const NOEXCEPT
{
    return {};
}

// server/electrum
TEMPLATE
code CLASS::get_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool /* turbo */) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    tx_links txs{};
    if (const auto ec = to_touched_txs(cancel, txs, outs))
        return ec;

    // TODO: parallel transform (requires vector), then sort/dedup.
    out.clear();
    for (const auto& tx: txs)
    {
        if (cancel)
            return error::canceled;

        // Handles terminal tx values.
        auto hash = get_tx_key(tx);
        if (hash == system::null_hash)
            return error::integrity;

        // Possibly missing prevout.
        uint64_t fee{};
        if (!get_tx_fee(fee, tx))
            return error::integrity;

        // Optimized by sharing strong across both subqueries.
        size_t height{}, position{};
        if (const auto strong = find_strong(tx); !strong.is_terminal())
        {
            if (!get_height(height, strong) ||
                !get_tx_position(position, tx, strong))
                return error::integrity;
        }
        else
        {
            height = is_confirmed_all_prevouts(tx) ? zero : max_size_t;
        }

        out.insert({ { std::move(hash), height }, fee, position });
    }

    return error::success;
}

// turbos
// ----------------------------------------------------------------------------
// protected

} // namespace database
} // namespace libbitcoin

#endif
