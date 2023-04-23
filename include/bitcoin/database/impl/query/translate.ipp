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
tx_link CLASS::to_input_tx(const input_link& link) const NOEXCEPT
{
    table::input::get_parent in{};
    if (!store_.input.get(link, in))
        return {};

    return in.parent_fk;
}

TEMPLATE
tx_link CLASS::to_output_tx(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    return out.parent_fk;
}

TEMPLATE
tx_link CLASS::to_prevout_tx(const input_link& link) const NOEXCEPT
{
    table::input::get_prevout in{};
    if (!store_.input.get(link, in) || in.is_null())
        return {};

    return to_tx(get_point_key(in.point_fk));
}

TEMPLATE
tx_link CLASS::to_spend_tx(const spend_link& link) const NOEXCEPT
{
    table::spend::record spend{};
    if (!store_.spend.get(link, spend))
        return {};

    return spend.tx_fk;
}

TEMPLATE
foreign_point CLASS::to_spend_key(const input_link& link) const NOEXCEPT
{
    table::input::get_prevout in{};
    if (!store_.input.get(link, in))
        return {};

    return in.prevout();
}

// point to put (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
input_link CLASS::to_input(const tx_link& link,
    uint32_t input_index) const NOEXCEPT
{
    table::transaction::get_input tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_at put{};
    if (!store_.puts.get(tx.puts_fk, put))
        return {};

    return put.put_fk;
}

TEMPLATE
output_link CLASS::to_output(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    table::transaction::get_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_at put{};
    if (!store_.puts.get(tx.puts_fk, put))
        return {};

    return put.put_fk;
}

TEMPLATE
output_link CLASS::to_prevout(const input_link& link) const NOEXCEPT
{
    table::input::get_prevout in{};
    if (!store_.input.get(link, in) || in.is_null())
        return {};

    return to_output(to_tx(get_point_key(in.point_fk)), in.point_index);
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
uint32_t CLASS::to_input_index(const tx_link& parent_fk,
    const input_link& input_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& in_fk: to_tx_inputs(parent_fk))
    {
        if (in_fk == input_fk) return index;
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
input_link CLASS::to_spender(const tx_link& link,
    const foreign_point& point) const NOEXCEPT
{
    table::input::get_prevout input{};
    for (const auto& in_fk: to_tx_inputs(link))
        if (store_.input.get(in_fk, input) && (input.prevout() == point))
            return in_fk;

    return {};
}

TEMPLATE
input_links CLASS::to_spenders(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    // This results in two reads to the tx table, so could be optimized.
    return to_spenders(out.parent_fk, to_output_index(out.parent_fk, link));
}

TEMPLATE
input_links CLASS::to_spenders(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders(point{ get_tx_key(link), output_index });
}

TEMPLATE
input_links CLASS::to_spenders(const point& prevout) const NOEXCEPT
{
    const auto point_fk = to_point(prevout.hash());
    if (point_fk.is_terminal())
        return {};

    return to_spenders(table::spend::compose(point_fk, prevout.index()));
}

TEMPLATE
input_links CLASS::to_spenders(const foreign_point& point) const NOEXCEPT
{
    auto it = store_.spend.it(point);
    if (it.self().is_terminal())
        return {};

    // Iterate transactions that spend the point, saving each spender.
    input_links spenders{};
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
input_links CLASS::to_tx_inputs(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_puts puts{};
    puts.put_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    return std::move(puts.put_fks);
}

TEMPLATE
output_links CLASS::to_tx_outputs(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::get_puts puts{};
    puts.put_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.outs_fk(), puts))
        return {};

    return std::move(puts.put_fks);
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
input_links CLASS::to_non_coinbase_inputs(
    const header_link& link) const NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.size() <= one)
        return {};

    input_links ins{};
    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
    {
        const auto inputs = to_tx_inputs(*tx);
        ins.insert(ins.end(), inputs.begin(), inputs.end());
    }

    return ins;
}

TEMPLATE
input_links CLASS::to_block_inputs(const header_link& link) const NOEXCEPT
{
    input_links ins{};
    const auto txs = to_txs(link);

    for (const auto& tx: txs)
    {
        const auto inputs = to_tx_inputs(tx);
        ins.insert(ins.end(), inputs.begin(), inputs.end());
    }

    return ins;
}

TEMPLATE
output_links CLASS::to_block_outputs(const header_link& link) const NOEXCEPT
{
    output_links outs{};
    const auto txs = to_txs(link);

    for (const auto& tx: txs)
    {
        const auto outputs = to_tx_outputs(tx);
        outs.insert(outs.end(), outputs.begin(), outputs.end());
    }

    return outs;
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
