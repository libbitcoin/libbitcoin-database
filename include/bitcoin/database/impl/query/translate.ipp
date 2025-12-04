/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_TRANSLATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_TRANSLATE_IPP

#include <algorithm>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// natural key (entry)
// ----------------------------------------------------------------------------

TEMPLATE
inline header_link CLASS::to_candidate(size_t height) const NOEXCEPT
{
    using namespace system;
    using link = table::height::header::integer;
    if (height >= store_.candidate.count())
        return {};

    table::height::record index{};
    if (!store_.candidate.get(possible_narrow_cast<link>(height), index))
        return {};

    return index.header_fk;
}

TEMPLATE
inline header_link CLASS::to_confirmed(size_t height) const NOEXCEPT
{
    using namespace system;
    using link = table::height::header::integer;
    if (height >= store_.confirmed.count())
        return {};

    table::height::record index{};
    if (!store_.confirmed.get(possible_narrow_cast<link>(height), index))
        return {};

    return index.header_fk;
}

TEMPLATE
inline header_link CLASS::to_header(const hash_digest& key) const NOEXCEPT
{
    return store_.header.first(key);
}

TEMPLATE
inline tx_link CLASS::to_tx(const hash_digest& key) const NOEXCEPT
{
    return store_.tx.first(key);
}

TEMPLATE
inline filter_link CLASS::to_filter(const header_link& key) const NOEXCEPT
{
    return store_.filter_tx.first(key);
}

TEMPLATE
output_link CLASS::to_output(const point& prevout) const NOEXCEPT
{
    return to_output(prevout.hash(), prevout.index());
}

TEMPLATE
output_link CLASS::to_output(const hash_digest& key,
    uint32_t input_index) const NOEXCEPT
{
    return to_output(to_tx(key), input_index);
}

// put to tx (reverse navigation)
// ----------------------------------------------------------------------------

TEMPLATE
tx_link CLASS::to_output_tx(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    return out.parent_fk;
}

TEMPLATE
tx_link CLASS::to_prevout_tx(const point_link& link) const NOEXCEPT
{
    return to_tx(get_point_hash(link));
}

TEMPLATE
tx_link CLASS::to_spending_tx(const point_link& link) const NOEXCEPT
{
    table::ins::get_parent ins{};
    if (!store_.ins.get(link, ins))
        return {};

    return ins.parent_fk;
}

// point to put (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
point_link CLASS::to_point(const tx_link& link,
    uint32_t input_index) const NOEXCEPT
{
    table::transaction::get_point tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    return tx.point_fk;
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
output_link CLASS::to_prevout(const point_link& link) const NOEXCEPT
{
    table::point::record point{};
    if (!store_.point.get(link, point))
        return {};

    return to_output(to_tx(point.hash), point.index);
}

// block/tx to block (reverse navigation)
// ----------------------------------------------------------------------------

// Required for confirmation processing.
TEMPLATE
header_link CLASS::to_block(const tx_link& key) const NOEXCEPT
{
    table::strong_tx::record strong{};
    if (!store_.strong_tx.find(key, strong) || !strong.positive())
        return {};

    // Terminal implies not in strong block (reorganized).
    return strong.header_fk();
}

// Required for confirmation processing.
TEMPLATE
header_link CLASS::to_strong(const hash_digest& tx_hash) const NOEXCEPT
{
    // Get all tx links for tx_hash.
    tx_links txs{};
    for (auto it = store_.tx.it(tx_hash); it; ++it)
        txs.push_back(*it);

    // Find the first strong tx of the set and return its block.
    for (const auto& tx: txs)
    {
        const auto block = to_block(tx);
        if (!block.is_terminal())
            return block;
    }

    return {};
}
TEMPLATE
header_link CLASS::to_parent(const header_link& link) const NOEXCEPT
{
    table::header::get_parent_fk header{};
    if (!store_.header.get(link, header))
        return {};

    // Terminal implies genesis (no parent).
    return header.parent_fk;
}

// to confirmed objects (reverse navigation)
// ----------------------------------------------------------------------------

TEMPLATE
header_link CLASS::to_confirmed_block(
    const hash_digest& tx_hash) const NOEXCEPT
{
    const auto block = to_strong(tx_hash);
    if (!is_confirmed_block(block))
        return {};

    return block;
}

TEMPLATE
point_link CLASS::to_confirmed_spender(const point&) const NOEXCEPT
{
    // TODO: implement.
    return {};
}

// output to spenders (reverse navigation)
// ----------------------------------------------------------------------------

// protected/unused (symmetry)
TEMPLATE
uint32_t CLASS::to_input_index(const tx_link& parent_fk,
    const point_link& point_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& in_fk: to_points(parent_fk))
    {
        if (in_fk == point_fk) return index;
        ++index;
    }

    return point::null_index;
}

// protected/to_spenders
TEMPLATE
uint32_t CLASS::to_output_index(const tx_link& parent_fk,
    const output_link& output_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& out_fk: to_outputs(parent_fk))
    {
        if (out_fk == output_fk) return index;
        ++index;
    }

    return point::null_index;
}

TEMPLATE
point_links CLASS::to_spenders(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    // This results in two reads to the tx table, so could be optimized.
    return to_spenders(out.parent_fk, to_output_index(out.parent_fk, link));
}

TEMPLATE
point_links CLASS::to_spenders(const tx_link& output_tx,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders(get_tx_key(output_tx), output_index);
}

TEMPLATE
point_links CLASS::to_spenders(const hash_digest& point_hash,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders({ point_hash, output_index });
}

TEMPLATE
point_links CLASS::to_spenders(const point& point) const NOEXCEPT
{
    // Avoid returning spend links for coinbase inputs (not spenders).
    if (point.index() == point::null_index)
        return {};

    point_links points{};
    for (auto it = store_.point.it(point); it; ++it)
        points.push_back(*it);

    return points;
}

// tx to puts (forward navigation)
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
        point = tx.point_fk++;

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

    // TODO: to_prevout()
    output_links prevouts{};
    prevouts.reserve(points.size());
    for (const auto& point: points)
        prevouts.push_back(to_prevout(point));

    return prevouts;
}

// txs to puts (forward navigation)
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
    constexpr auto parallel = poolstl::execution::par;

    // to_prevout()
    const auto ins = to_points(txs);
    output_links outs(ins.size());
    const auto fn = [this](auto spend) NOEXCEPT{ return to_prevout(spend); };
    std::transform(parallel, ins.begin(), ins.end(), outs.begin(), fn);
    return outs;
}

// block to puts (forward navigation)
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

// block to txs (forward navigation)
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

// header to arraymap tables (guard domain transitions)
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::to_validated_bk(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::validated_bk::link::terminal);
    return link.is_terminal() ? table::validated_bk::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_filter_bk(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::filter_bk::link::terminal);
    return link.is_terminal() ? table::filter_bk::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_filter_tx(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::filter_tx::link::terminal);
    return link.is_terminal() ? table::filter_tx::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_prevout(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::prevout::link::terminal);
    return link.is_terminal() ? table::prevout::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_txs(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::txs::link::terminal);
    return link.is_terminal() ? table::txs::link::terminal : link.value;
}

// hashmap enumeration
// ----------------------------------------------------------------------------

TEMPLATE
header_link CLASS::top_header(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.header.top(possible_narrow_cast<header_link::integer>(bucket));
}

TEMPLATE
point_link CLASS::top_point(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.point.top(possible_narrow_cast<point_link::integer>(bucket));
}

TEMPLATE
tx_link CLASS::top_tx(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.tx.top(possible_narrow_cast<tx_link::integer>(bucket));
}

} // namespace database
} // namespace libbitcoin

#endif
