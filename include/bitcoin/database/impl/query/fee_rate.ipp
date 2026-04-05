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
#ifndef LIBBITCOIN_DATABASE_QUERY_FEE_RATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_FEE_RATE_IPP

#include <atomic>
#include <algorithm>
#include <iterator>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// tx/block/branch fee rates
// ----------------------------------------------------------------------------
// server estimator

TEMPLATE
bool CLASS::get_tx_fees(fee_rate& out, const tx_link& link) const NOEXCEPT
{
    // This is somehow ~15-20% less efficient.
    ////return get_tx_virtual_size(out.bytes, link) && get_tx_fee(out.fee, link);
    const auto tx = get_transaction(link, false);
    if (!tx || tx->is_coinbase() || !populate_without_metadata(*tx))
        return false;

    out.bytes = tx->virtual_size();
    out.fee = tx->fee();
    return true;
}

TEMPLATE
bool CLASS::get_block_fees(fee_rates& out,
    const header_link& link) const NOEXCEPT
{
    out.clear();
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || (txs.tx_fks.size() < one))
        return false;

    out.resize(sub1(txs.tx_fks.size()));
    const auto end = txs.tx_fks.end();
    auto rate = out.begin();

    // Skip coinbase.
    for (auto tx = std::next(txs.tx_fks.begin()); tx != end; ++tx)
        if (!get_tx_fees(*rate++, *tx))
            return false;

    return true;
}

TEMPLATE
bool CLASS::get_branch_fees(std::atomic_bool& cancel, fee_rate_sets& out,
    size_t start, size_t count) const NOEXCEPT
{
    out.clear();
    if (is_zero(count))
        return true;

    if (system::is_add_overflow(start, sub1(count)) ||
        (start + sub1(count) > get_top_confirmed()))
        return false;

    out.resize(count);
    std::atomic_bool fail{};
    std::vector<size_t> offsets(count);
    std::iota(offsets.begin(), offsets.end(), zero);
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    // Parallel execution saves ~50%.
    std::for_each(parallel, offsets.begin(), offsets.end(),
        [&](const size_t& offset) NOEXCEPT
        {
            if (fail.load(relaxed))
                return;

            if (cancel.load(relaxed) || !get_block_fees(out.at(offset),
                to_confirmed(start + offset)))
                fail.store(true, relaxed);
        });

    const auto failed = fail.load(relaxed);
    if (failed) out.clear();
    return !failed;
}

} // namespace database
} // namespace libbitcoin

#endif
