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
#ifndef LIBBITCOIN_DATABASE_QUERY_AMOUNTS_IPP
#define LIBBITCOIN_DATABASE_QUERY_AMOUNTS_IPP

#include <atomic>
#include <algorithm>
#include <iterator>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// block/tx/output value, spend, fee
// ----------------------------------------------------------------------------

// unused
TEMPLATE
bool CLASS::get_value(uint64_t& out, const output_link& link) const NOEXCEPT
{
    table::output::get_value output{};
    if (!store_.output.get(link, output))
        return false;

    out = output.value;
    return true;
}

// server/native
TEMPLATE
bool CLASS::get_tx_value(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    table::transaction::get_coinbase tx{};
    if (!store_.tx.get(link, tx))
        return false;

    // Shortcircuit coinbase prevout read.
    if (tx.coinbase)
    {
        out = zero;
        return true;
    }

    // Optimizable due to sequential tx input links.
    const auto links = to_prevouts(link);
    return !links.empty() && get_outputs_total_value(out, links);
}

// server/native
TEMPLATE
bool CLASS::get_tx_spend(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    const auto links = to_outputs(link);
    return !links.empty() && get_outputs_total_value(out, links);
}

// unused (disabled in get_tx_fees())
TEMPLATE
bool CLASS::get_tx_fee(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    uint64_t value{};
    if (!get_tx_value(value, link))
        return false;

    // Zero input implies either zero output or coinbase (both zero).
    if (is_zero(value))
        return true;

    uint64_t spend{};
    if (!get_tx_spend(spend, link) || spend > value)
        return false;

    out = value - spend;
    return true;
}

// server/native
TEMPLATE
bool CLASS::get_block_value(uint64_t& out,
    const header_link& link) const NOEXCEPT
{
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || (txs.tx_fks.size() < one))
        return false;

    std::atomic_bool fail{};
    const auto begin = std::next(txs.tx_fks.begin());
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    out = std::transform_reduce(parallel, begin, txs.tx_fks.end(), 0_u64,
        [](uint64_t left, uint64_t right) NOEXCEPT
        {
            return system::ceilinged_add(left, right);
        },
        [&](const auto& tx_fk) NOEXCEPT
        {
            uint64_t value{};
            if (!fail.load(relaxed) && !get_tx_value(value, tx_fk))
                fail.store(true, relaxed);

            return value;
        });

    return !fail.load(relaxed);
}

// server/native
TEMPLATE
bool CLASS::get_block_spend(uint64_t& out,
    const header_link& link) const NOEXCEPT
{
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || (txs.tx_fks.size() < one))
        return false;

    std::atomic_bool fail{};
    const auto begin = std::next(txs.tx_fks.begin());
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    out = std::transform_reduce(parallel, begin, txs.tx_fks.end(), 0_u64,
        [](uint64_t left, uint64_t right) NOEXCEPT
        {
            return system::ceilinged_add(left, right);
        },
        [&](const auto& tx_fk) NOEXCEPT
        {
            uint64_t spend{};
            if (!fail.load(relaxed) && !get_tx_spend(spend, tx_fk))
                fail.store(true, relaxed);

            return spend;
        });

    return !fail.load(relaxed);
}

// unused
TEMPLATE
bool CLASS::get_block_fee(uint64_t& out,
    const header_link& link) const NOEXCEPT
{
    uint64_t value{}, spend{};
    if (!get_block_value(value, link) || !get_block_spend(spend, link) ||
        spend > value)
        return false;

    out = value - spend;
    return true;
}

// utility
// ----------------------------------------------------------------------------

// protected
TEMPLATE
bool CLASS::get_outputs_total_value(uint64_t& out,
    const output_links& links) const NOEXCEPT
{
    out = zero;
    for (const auto& output_fk: links)
    {
        uint64_t value{};
        if (!get_value(value, output_fk)) return false;
        out = system::ceilinged_add(out, value);
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
