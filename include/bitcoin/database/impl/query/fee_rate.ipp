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
#ifndef LIBBITCOIN_DATABASE_QUERY_FEE_RATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_FEE_RATE_IPP

#include <atomic>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// tx/block/branch fee rates
// ----------------------------------------------------------------------------
// server estimator

TEMPLATE
bool CLASS::get_tx_fees(fee_rate& out, const tx_link& link) const NOEXCEPT
{
    if (get_pooled_fee(out.fee, out.height, link))
        return get_tx_virtual_size(out.bytes, link);

    // This is somehow ~15-20% less efficient.
    ////return get_tx_virtual_size(out.bytes, link) && get_tx_fee(out.fee, link);
    const auto tx = get_transaction(link, false);
    if (!tx || tx->is_coinbase() || !populate_without_metadata(*tx))
        return false;

    out.bytes = tx->virtual_size();
    out.fee = tx->fee();
    out.height = max_size_t;
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
    const auto end = txs.tx_fks.cend();
    auto rate = out.begin();

    // Skip coinbase.
    for (auto tx = std::next(txs.tx_fks.cbegin()); tx != end; ++tx)
        if (!get_tx_fees(*rate++, *tx))
            return false;

    return true;
}

TEMPLATE
bool CLASS::get_branch_fees(const stopper& cancel, fee_rate_sets& out,
    size_t start, size_t count) const NOEXCEPT
{
    out.clear();
    if (is_zero(count))
        return true;

    if (system::is_add_overflow(start, sub1(count)) ||
        (start + sub1(count) > get_top_confirmed()))
        return false;

    out.resize(count);
    stopper fail{};
    std::vector<size_t> it(count);
    std::iota(it.begin(), it.end(), zero);
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    // Parallel execution saves ~50%.
    std::for_each(parallel, it.cbegin(), it.cend(), [&](size_t offset) NOEXCEPT
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

// The block's own txs are excluded.
TEMPLATE
bool CLASS::get_block_conflict_fees(fee_rates& out,
    const header_link& link) const NOEXCEPT
{
    out.clear();
    if (!store_.pool.enabled())
        return true;

    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    const std::unordered_set<tx_link::integer> block
    {
        txs.cbegin(), txs.cend()
    };

    std::vector<tx_link::integer> pending{};
    std::unordered_set<tx_link::integer> visited{};
    const auto add = [&](const tx_link& tx) NOEXCEPT
    {
        if (!block.contains(tx.value) && is_pooled(tx) &&
            visited.insert(tx.value).second)
            pending.push_back(tx.value);
    };

    for (const auto& tx: txs)
    {
        table::transaction::only record{};
        if (!store_.tx.get(tx, record))
            return false;

        const auto end = record.point_fk + record.ins_count;
        for (auto fk = record.point_fk; fk < end; ++fk)
            if (const auto prevout = get_point_key(fk); !prevout.is_null())
                for (const auto& in: to_spenders(prevout))
                    add(to_input_tx(in));
    }

    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();

        table::transaction::only record{};
        if (!store_.tx.get(next, record) ||
            !get_tx_fees(out.emplace_back(), next))
            return false;

        for (uint32_t index{}; index < record.outs_count; ++index)
            for (const auto& in: to_spenders(next, index))
                add(to_input_tx(in));
    }

    return true;
}

TEMPLATE
bool CLASS::get_confirmable_fees(fee_rates& out) const NOEXCEPT
{
    out.clear();
    if (!store_.pool.enabled())
        return true;

    confirmables memo{};
    const auto rows = store_.pool.count();
    for (table::pool::link::integer row{}; row < rows; ++row)
    {
        const tx_link link{ store_.pool.get_key(row) };
        if (store_.pool.first(link) != row || !is_confirmable_tx(memo, link))
            continue;

        if (!get_tx_fees(out.emplace_back(), link))
            return false;
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
