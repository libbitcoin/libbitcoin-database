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
code CLASS::get_unconfirmed_balance(std::atomic_bool& , uint64_t& ,
    const hash_digest& , bool ) const NOEXCEPT
{
    return {};
}

// server/native
TEMPLATE
code CLASS::get_confirmed_balance(std::atomic_bool& cancel, uint64_t& out,
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
code CLASS::get_balance(std::atomic_bool& , uint64_t& ,
    uint64_t& , const hash_digest& ,
    bool ) const NOEXCEPT
{
    return {};
}

} // namespace database
} // namespace libbitcoin

#endif
