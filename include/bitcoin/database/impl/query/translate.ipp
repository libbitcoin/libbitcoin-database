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
    using link = table::height::block::integer;
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
    using link = table::height::block::integer;
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
    // Use header.find(key) in place of get(to_header(key)).
    return store_.header.first(key);
}

TEMPLATE
inline point_link CLASS::to_point(const hash_digest& key) const NOEXCEPT
{
    // Use point.find(key) in place of get(to_point(key)).
    return store_.point.first(key);
}

TEMPLATE
inline tx_link CLASS::to_tx(const hash_digest& key) const NOEXCEPT
{
    // Use tx.find(key) in place of get(to_tx(key)).
    return store_.tx.first(key);
}

TEMPLATE
inline txs_link CLASS::to_txs(const header_link& key) const NOEXCEPT
{
    // Use txs.find(key) in place of get(to_txs(key)).
    return store_.txs.first(key);
}

TEMPLATE
inline filter_link CLASS::to_filter(const header_link& key) const NOEXCEPT
{
    // Use neutrino.find(key) in place of get(to_filter(key)).
    return store_.neutrino.first(key);
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
tx_link CLASS::to_prevout_tx(const spend_link& link) const NOEXCEPT
{
    table::spend::get_point spend{};
    if (!store_.spend.get(link, spend) || spend.is_null())
        return {};

    return to_tx(get_point_key(spend.point_fk));
}

TEMPLATE
tx_link CLASS::to_spend_tx(const spend_link& link) const NOEXCEPT
{
    table::spend::get_parent spend{};
    if (!store_.spend.get(link, spend))
        return {};

    return spend.parent_fk;
}

TEMPLATE
spend_key CLASS::to_spend_key(const spend_link& link) const NOEXCEPT
{
    table::spend::get_key spend{};
    if (!store_.spend.get(link, spend))
        return {};

    return spend.key;
}

// point to put (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
spend_link CLASS::to_spend(const tx_link& link,
    uint32_t spend_index) const NOEXCEPT
{
    table::transaction::get_spend tx{ {}, spend_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_spend_at put{};
    if (!store_.puts.get(tx.spend_fk, put))
        return {};

    return put.spend_fk;
}

TEMPLATE
output_link CLASS::to_output(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    table::transaction::get_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_output_at put{};
    if (!store_.puts.get(tx.out_fk, put))
        return {};

    return put.out_fk;
}

TEMPLATE
output_link CLASS::to_prevout(const spend_link& link) const NOEXCEPT
{
    table::spend::get_prevout spend{};
    if (!store_.spend.get(link, spend) || spend.is_null())
        return {};

    return to_output(to_tx(get_point_key(spend.point_fk)), spend.point_index);
}

// block/tx to block (reverse navigation)
// ----------------------------------------------------------------------------
// Required for confirmation processing.

TEMPLATE
header_link CLASS::to_parent(const header_link& link) const NOEXCEPT
{
    table::header::get_parent_fk header{};
    if (!store_.header.get(link, header))
        return {};

    // Terminal implies genesis (no parent).
    return header.parent_fk;
}

TEMPLATE
header_link CLASS::to_block(const tx_link& key) const NOEXCEPT
{
    table::strong_tx::record strong{};
    if (!store_.strong_tx.find(key, strong) || !strong.positive)
        return {};

    // Terminal implies not in strong block (reorganized).
    return strong.header_fk;
}

// protected
// If there are no associations the link of the first tx by hash is returned,
// which is an optimization to prevent requery to determine tx existence.
// Return the first block-tx tuple where the tx is strong by the block.
TEMPLATE
inline strong_pair CLASS::to_strong(const hash_digest& tx_hash) const NOEXCEPT
{
    // Iteration of tx is necessary because there may be duplicates.
    auto it = store_.tx.it(tx_hash);
    strong_pair strong{ {}, it.self() };
    if (!it)
        return strong;

    // TODO: deadlock risk.
    do
    {
        // Only top block (strong) association for given tx is considered.
        strong.block = to_block(strong.tx);
        if (!strong.block.is_terminal())
        {
            strong.tx = it.self();
            return strong;
        }
    }
    while (it.advance());
    return strong;
}

// output to spenders (reverse navigation)
// ----------------------------------------------------------------------------

// protected/unused (symmetry)
TEMPLATE
uint32_t CLASS::to_spend_index(const tx_link& parent_fk,
    const spend_link& spend_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& in_fk: to_spends(parent_fk))
    {
        if (in_fk == spend_fk) return index;
        ++index;
    }

    return system::chain::point::null_index;
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

    return system::chain::point::null_index;
}

// Assumes singular which doesn't make sense.
// protected/to_spenders
////TEMPLATE
////spend_link CLASS::to_spender(const tx_link& link,
////    const spend_key& point) const NOEXCEPT
////{
////    table::spend::get_key spend{};
////    for (const auto& spend_fk: to_spends(link))
////        if (store_.spend.get(spend_fk, spend) && (spend.key == point))
////            return spend_fk;
////
////    return {};
////}

TEMPLATE
spend_links CLASS::to_spenders(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    // This results in two reads to the tx table, so could be optimized.
    return to_spenders(out.parent_fk, to_output_index(out.parent_fk, link));
}

TEMPLATE
spend_links CLASS::to_spenders(const point& point) const NOEXCEPT
{
    return to_spenders(point.hash(), point.index());
}

TEMPLATE
spend_links CLASS::to_spenders(const tx_link& output_tx,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders(get_tx_key(output_tx), output_index);
}

TEMPLATE
spend_links CLASS::to_spenders(const hash_digest& point_hash,
    uint32_t output_index) const NOEXCEPT
{
    // Avoid returning spend links for coinbase inputs.
    if (output_index == system::chain::point::null_index)
        return {};

    // This will find null points, as every input is spend-table-indexed.
    // Iterates the set of possible matches to point (conflicts).
    auto it = store_.spend.it(table::spend::compose(point_hash, output_index));
    if (!it)
        return {};

    // For a null point this will obtain terminal point_fk.
    potentials potentials{};
    do
    {
        table::spend::get_point spend{};
        if (!store_.spend.get(it, spend))
            return {};

        potentials.emplace_back(it.self(), spend.point_fk);
    }
    while (it.advance());
    it.reset();

    // A terminal point_fk will fail get_point_key with a null_hash return.
    // A secondary match must be made against the point table hash value.
    spend_links links{};
    for (const auto& potential: potentials)
        if (get_point_key(potential.point_fk) == point_hash)
            links.push_back(potential.spend_fk);

    return links;
}

// tx to puts (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
spend_links CLASS::to_spends(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_spends puts{};
    puts.spend_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.puts_fk, puts))
        return {};

    return std::move(puts.spend_fks);
}

TEMPLATE
output_links CLASS::to_outputs(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_outs puts{};
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.outs_fk(), puts))
        return {};

    return std::move(puts.out_fks);
}

TEMPLATE
output_links CLASS::to_prevouts(const tx_link& link) const NOEXCEPT
{
    const auto spends = to_spends(link);
    if (spends.empty())
        return {};

    output_links prevouts{};
    prevouts.reserve(spends.size());
    for (const auto& spend: spends)
        prevouts.push_back(to_prevout(spend));

    return prevouts;
}

// txs to puts (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
spend_links CLASS::to_spends(const tx_links& txs) const NOEXCEPT
{
    spend_links spends{};
    for (const auto& tx: txs)
    {
        const auto tx_spends = to_spends(tx);
        spends.insert(spends.end(), tx_spends.begin(), tx_spends.end());
    }

    return spends;
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
    const auto ins = to_spends(txs);
    output_links outs(ins.size());
    const auto fn = [this](auto spend) NOEXCEPT{ return to_prevout(spend); };

    // C++17 incomplete on GCC/CLang, so presently parallel only on MSVC++.
    std_transform(bc::par_unseq, ins.begin(), ins.end(), outs.begin(), fn);
    return outs;
}

// block to puts (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
spend_links CLASS::to_block_spends(const header_link& link) const NOEXCEPT
{
    return to_spends(to_spending_transactions(link));
}

TEMPLATE
output_links CLASS::to_block_outputs(const header_link& link) const NOEXCEPT
{
    return to_outputs(to_transactions(link));
}

TEMPLATE
output_links CLASS::to_block_prevouts(const header_link& link) const NOEXCEPT
{
    return to_prevouts(to_spending_transactions(link));
}

// block to txs (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
tx_link CLASS::to_coinbase(const header_link& link) const NOEXCEPT
{
    table::txs::get_coinbase txs{};
    if (!store_.txs.find(link, txs))
        return {};

    return txs.coinbase_fk;
}

TEMPLATE
tx_links CLASS::to_transactions(const header_link& link) const NOEXCEPT
{
    table::txs::get_txs txs{};
    if (!store_.txs.find(link, txs))
        return {};

    return std::move(txs.tx_fks);
}

TEMPLATE
tx_links CLASS::to_spending_transactions(const header_link& link) const NOEXCEPT
{
    table::txs::get_spending_txs txs{};
    if (!store_.txs.find(link, txs))
        return {};

    return std::move(txs.tx_fks);
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
spend_link CLASS::top_spend(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.spend.top(possible_narrow_cast<spend_link::integer>(bucket));
}

TEMPLATE
txs_link CLASS::top_txs(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.txs.top(possible_narrow_cast<txs_link::integer>(bucket));
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
