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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_BALANCE_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_BALANCE_IPP

#include <atomic>
#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address balance
// ----------------------------------------------------------------------------
// Balance queries (universal, unconfirmed conflict resolution arbitrary).

// unused
TEMPLATE
code CLASS::get_unconfirmed_balance(stopper& cancel, uint64_t& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    // While duplicates are easily filtered out, conflict resolution is murky.
    // An output may have multiple directly or indirectly conflicting spends,
    // and other spends and receives may not be visible. An unconfirmed balance
    // is therefore inherehtly ambiguous. Given the lack of tx pooling,
    // presently we just return combined = confirmed (net zero unconfirmed).
    return get_confirmed_balance(cancel, out, key, turbo);
}

// server/native
TEMPLATE
code CLASS::get_confirmed_balance(stopper& cancel, uint64_t& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    outpoints outs{};
    if (const auto ec = get_confirmed_unspent_outputs(cancel, outs, key, turbo))
    {
        out = zero;
        return ec;
    }

    // Use of to_confirmed_unspent_outputs() provides necessary deduplication.
    out = std::accumulate(outs.begin(), outs.end(), zero,
        [](size_t total, const outpoint& out) NOEXCEPT
        {
            return system::ceilinged_add(total, out.value());
        });

    return error::success;
}

// server/electrum
TEMPLATE
code CLASS::get_balance(stopper& cancel, uint64_t& confirmed,
    uint64_t& combined, const hash_digest& key, bool turbo) const NOEXCEPT
{
    // See notes on get_unconfirmed_balance().
    const auto ec = get_confirmed_balance(cancel, confirmed, key, turbo);
    combined = confirmed;
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
