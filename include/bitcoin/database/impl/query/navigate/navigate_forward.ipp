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
#ifndef LIBBITCOIN_DATABASE_QUERY_FORWARD_IPP
#define LIBBITCOIN_DATABASE_QUERY_FORWARD_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// [tx, index]->input|output|prevout
// ----------------------------------------------------------------------------

TEMPLATE
point_link CLASS::to_point(const tx_link& link,
    uint32_t input_index) const NOEXCEPT
{
    table::transaction::get_point tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    return tx.points_fk;
}

TEMPLATE
output_link CLASS::to_output(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    table::transaction::get_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::outs::get_output outs{};
    if (!store_.outs.get(tx.outs_fk, outs))
        return {};

    return outs.out_fk;
}

TEMPLATE
output_link CLASS::to_previous_output(const point_link& link) const NOEXCEPT
{
    return to_output(get_point_key(link));
}

// tx->inputs|outputs|prevouts
// ----------------------------------------------------------------------------

TEMPLATE
point_links CLASS::to_points(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_point tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // Transaction points are stored in a contiguous array of records.
    point_links points(tx.number);
    for (auto& point: points)
        point = tx.points_fk++;

    return points;
}

TEMPLATE
output_links CLASS::to_outputs(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_output tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::outs::record outs{};
    outs.out_fks.resize(tx.number);
    if (!store_.outs.get(tx.outs_fk, outs))
        return {};

    return std::move(outs.out_fks);
}

TEMPLATE
output_links CLASS::to_prevouts(const tx_link& link) const NOEXCEPT
{
    const auto points = to_points(link);
    if (points.empty())
        return {};

    output_links prevouts{};
    prevouts.reserve(points.size());
    for (const auto& point: points)
        prevouts.push_back(to_previous_output(point));

    return prevouts;
}

// block.txs->inputs|outputs|prevouts
// ----------------------------------------------------------------------------

// to_ins()
TEMPLATE
point_links CLASS::to_points(const tx_links& txs) const NOEXCEPT
{
    point_links points{};
    for (const auto& tx: txs)
    {
        const auto tx_points = to_points(tx);
        points.insert(points.end(), tx_points.begin(), tx_points.end());
    }

    return points;
}

TEMPLATE
output_links CLASS::to_outputs(const tx_links& txs) const NOEXCEPT
{
    output_links outputs{};
    for (const auto& tx: txs)
    {
        const auto tx_outputs = to_outputs(tx);
        outputs.insert(outputs.end(), tx_outputs.begin(), tx_outputs.end());
    }

    return outputs;
}

TEMPLATE
output_links CLASS::to_prevouts(const tx_links& txs) const NOEXCEPT
{
    const auto ins = to_points(txs);
    output_links outs(ins.size());
    constexpr auto parallel = poolstl::execution::par;

    std::transform(parallel, ins.begin(), ins.end(), outs.begin(),
        [&](const auto& spend) NOEXCEPT
        {
            return to_previous_output(spend);
        });

    return outs;
}

// block->inputs|outputs|prevouts
// ----------------------------------------------------------------------------

TEMPLATE
point_links CLASS::to_block_points(const header_link& link) const NOEXCEPT
{
    return to_points(to_spending_txs(link));
}

TEMPLATE
output_links CLASS::to_block_outputs(const header_link& link) const NOEXCEPT
{
    return to_outputs(to_transactions(link));
}

TEMPLATE
output_links CLASS::to_block_prevouts(const header_link& link) const NOEXCEPT
{
    return to_prevouts(to_spending_txs(link));
}

// block->txs|txs[!cb]|cb|tx[@postion]
// ----------------------------------------------------------------------------

TEMPLATE
tx_links CLASS::to_transactions(const header_link& link) const NOEXCEPT
{
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return std::move(txs.tx_fks);
}

TEMPLATE
tx_links CLASS::to_spending_txs(const header_link& link) const NOEXCEPT
{
    table::txs::get_spending_txs txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return std::move(txs.tx_fks);
}

TEMPLATE
tx_link CLASS::to_coinbase(const header_link& link) const NOEXCEPT
{
    table::txs::get_coinbase txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return txs.coinbase_fk;
}

TEMPLATE
tx_link CLASS::to_transaction(const header_link& link,
    size_t position) const NOEXCEPT
{
    table::txs::get_tx txs{ {}, position };
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return txs.tx_fk;
}

} // namespace database
} // namespace libbitcoin

#endif
