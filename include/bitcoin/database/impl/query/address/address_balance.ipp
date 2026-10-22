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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_BALANCE_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_BALANCE_IPP

#include <atomic>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address balance
// ----------------------------------------------------------------------------
// Balance queries (universal, unconfirmed conflict resolution arbitrary).

// unused
TEMPLATE
code CLASS::get_unconfirmed_balance(const stopper& , uint64_t& out,
    const hash_digest& , bool ) const NOEXCEPT
{
    // While duplicates are easily filtered out, conflict resolution is murky.
    // An output may have multiple directly or indirectly conflicting spends,
    // and other spends and receives may not be visible. An unconfirmed balance
    // is therefore inherehtly ambiguous. Given the lack of tx pooling,
    // presently we just return zero unconfirmed.
    out = zero;
    return error::success;
}

// server/native
TEMPLATE
code CLASS::get_confirmed_balance(const stopper& cancel, uint64_t& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    outpoints outs{};
    if (const auto ec = get_confirmed_unspent_outpoints(cancel, outs, key,
        turbo))
    {
        out = zero;
        return ec;
    }

    // Use of to_confirmed_unspent_outputs() provides necessary deduplication.
    out = std::accumulate(outs.cbegin(), outs.cend(), zero,
        [](size_t total, const outpoint& out) NOEXCEPT
        {
            return system::ceilinged_add(total, out.value());
        });

    return error::success;
}

// server/electrum
TEMPLATE
code CLASS::get_balance(const stopper& cancel, uint64_t& confirmed,
    uint64_t& unconfirmed, const hash_digest& key, bool turbo) const NOEXCEPT
{
    // See notes on get_unconfirmed_balance().
    const auto ec = get_confirmed_balance(cancel, confirmed, key, turbo);
    unconfirmed = zero;
    return ec;
}

// server/esplora
TEMPLATE
code CLASS::get_address_totals(const stopper& cancel,
    address_totals& confirmed, address_totals& unconfirmed,
    const hash_digest& key) const NOEXCEPT
{
    using namespace system;
    confirmed = {};
    unconfirmed = {};
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    const auto fund = [](address_totals& totals, uint64_t value) NOEXCEPT
    {
        ++totals.funded_count;
        totals.funded_value = ceilinged_add(totals.funded_value, value);
    };

    const auto spend = [](address_totals& totals, uint64_t value) NOEXCEPT
    {
        ++totals.spent_count;
        totals.spent_value = ceilinged_add(totals.spent_value, value);
    };

    for (const auto& out: outs)
    {
        if (cancel)
            return error::query_canceled;

        uint64_t value{};
        if (!get_value(value, out))
            return error::integrity;

        if (is_confirmed_output(out))
        {
            fund(confirmed, value);
            if (is_confirmed_spent(out))
                spend(confirmed, value);
            else if (is_spent(out))
                spend(unconfirmed, value);
        }
        else
        {
            fund(unconfirmed, value);
            if (is_spent(out))
                spend(unconfirmed, value);
        }
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
