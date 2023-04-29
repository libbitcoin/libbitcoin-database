/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
    return store_.header.first(key);
}

TEMPLATE
inline point_link CLASS::to_point(const hash_digest& key) const NOEXCEPT
{
    return store_.point.first(key);
}

TEMPLATE
inline tx_link CLASS::to_tx(const hash_digest& key) const NOEXCEPT
{
    return store_.tx.first(key);
}

TEMPLATE
inline txs_link CLASS::to_txs_link(const header_link& link) const NOEXCEPT
{
    return store_.txs.first(link);
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
foreign_point CLASS::to_spend_key(const spend_link& link) const NOEXCEPT
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

TEMPLATE
header_link CLASS::to_block(const tx_link& link) const NOEXCEPT
{
    table::strong_tx::record strong{};
    if (!store_.strong_tx.get(store_.strong_tx.first(link), strong))
        return {};

    return strong.header_fk;
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

// output to spenders (reverse navigation)
// ----------------------------------------------------------------------------

// protected/unused (symmetry)
TEMPLATE
uint32_t CLASS::to_spend_index(const tx_link& parent_fk,
    const spend_link& spend_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& in_fk: to_tx_spends(parent_fk))
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
    for (const auto& out_fk: to_tx_outputs(parent_fk))
    {
        if (out_fk == output_fk) return index;
        ++index;
    }

    return system::chain::point::null_index;
}

// protected/to_spenders
TEMPLATE
spend_link CLASS::to_spender(const tx_link& link,
    const foreign_point& point) const NOEXCEPT
{
    table::spend::get_key spend{};
    for (const auto& spend_fk: to_tx_spends(link))
        if (store_.spend.get(spend_fk, spend) && (spend.key == point))
            return spend_fk;

    return {};
}

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
spend_links CLASS::to_spenders(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders(point{ get_tx_key(link), output_index });
}

TEMPLATE
spend_links CLASS::to_spenders(const point& prevout) const NOEXCEPT
{
    const auto point_fk = to_point(prevout.hash());
    if (point_fk.is_terminal())
        return {};

    return to_spenders(table::spend::compose(point_fk, prevout.index()));
}

TEMPLATE
spend_links CLASS::to_spenders(const foreign_point& point) const NOEXCEPT
{
    auto it = store_.spend.it(point);
    if (it.self().is_terminal())
        return {};

    // Iterate transactions that spend the point, saving each spender.
    spend_links spenders{};
    do
    {
        spenders.push_back(to_spender(to_spend_tx(it.self()), point));
    }
    while (it.advance());

    // Any terminal link implies a store integrity failure.
    return spenders;
}

// tx to puts (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
spend_links CLASS::to_tx_spends(const tx_link& link) const NOEXCEPT
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
output_links CLASS::to_tx_outputs(const tx_link& link) const NOEXCEPT
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

// block to txs/puts (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
tx_links CLASS::to_txs(const header_link& link) const NOEXCEPT
{
    table::txs::slab txs{};
    if (!store_.txs.get(to_txs_link(link), txs))
        return {};

    return std::move(txs.tx_fks);
}

TEMPLATE
tx_link CLASS::to_coinbase(const header_link& link) const NOEXCEPT
{
    table::txs::get_coinbase txs{};
    if (!store_.txs.get(to_txs_link(link), txs))
        return {};

    return txs.coinbase_fk;
}

TEMPLATE
spend_links CLASS::to_non_coinbase_spends(
    const header_link& link) const NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.size() <= one)
        return {};

    // Dynamic spends allocation is an unnecessary block_confirmable cost.
    spend_links spends{};
    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
    {
        const auto tx_spends = to_tx_spends(*tx);
        spends.insert(spends.end(), tx_spends.begin(), tx_spends.end());
    }

    return spends;
}

TEMPLATE
spend_links CLASS::to_block_spends(const header_link& link) const NOEXCEPT
{
    spend_links spends{};
    const auto txs = to_txs(link);

    for (const auto& tx: txs)
    {
        const auto tx_spends = to_tx_spends(tx);
        spends.insert(spends.end(), tx_spends.begin(), tx_spends.end());
    }

    return spends;
}

TEMPLATE
output_links CLASS::to_block_outputs(const header_link& link) const NOEXCEPT
{
    output_links outputs{};
    const auto txs = to_txs(link);

    for (const auto& tx: txs)
    {
        const auto tx_outputs = to_tx_outputs(tx);
        outputs.insert(outputs.end(), tx_outputs.begin(), tx_outputs.end());
    }

    return outputs;
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
