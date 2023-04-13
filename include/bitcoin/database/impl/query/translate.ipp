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

// protected
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
    table::input::slab_decomposed_fk in{};
    if (!store_.input.get(link, in))
        return {};

    if (in.is_null())
        return {};

    return to_tx(get_point_key(in.point_fk));
}

// point to put (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
input_link CLASS::to_input(const tx_link& link,
    uint32_t input_index) const NOEXCEPT
{
    table::transaction::record_input tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record_get_one put{};
    if (!store_.puts.get(tx.puts_fk, put))
        return {};

    return put.put_fk;
}

TEMPLATE
output_link CLASS::to_output(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    table::transaction::record_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record_get_one put{};
    if (!store_.puts.get(tx.puts_fk, put))
        return {};

    return put.put_fk;
}

TEMPLATE
output_link CLASS::to_prevout(const input_link& link) const NOEXCEPT
{
    table::input::slab_decomposed_sk in{};
    if (!store_.input.get(link, in))
        return {};

    if (in.is_null())
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

TEMPLATE
input_links CLASS::to_spenders(const output_link& link) const NOEXCEPT
{
    table::output::get_point out{};
    if (!store_.output.get(link, out))
        return {};

    // An output parent tx/index should never be null point.
    return to_spenders(out.parent_fk, out.index);
}

TEMPLATE
input_links CLASS::to_spenders(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto point_fk = to_point(get_tx_key(link));
    if (point_fk.is_terminal())
        return {};

    return to_spenders(table::input::compose(point_fk, output_index));
}

TEMPLATE
input_links CLASS::to_spenders(const point& prevout) const NOEXCEPT
{
    if (prevout.is_null())
        return {};

    const auto point_fk = to_point(prevout.hash());
    if (point_fk.is_terminal())
        return {};

    return to_spenders(table::input::compose(point_fk, prevout.index()));
}

// protected
TEMPLATE
input_links CLASS::to_spenders(
    const table::input::search_key& key) const NOEXCEPT
{
    auto it = store_.input.it(key);
    if (it.self().is_terminal())
        return {};

    // Spender count is low, so no parallel here.
    input_links spenders;
    do { spenders.push_back(it.self()); } while (it.advance());
    return spenders;
}

// tx to puts (forward navigation)
// ----------------------------------------------------------------------------

TEMPLATE
input_links CLASS::to_tx_inputs(const tx_link& link) const NOEXCEPT
{
    table::transaction::record_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record puts{};
    puts.in_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    return std::move(puts.in_fks);
}

TEMPLATE
output_links CLASS::to_tx_outputs(const tx_link& link) const NOEXCEPT
{
    table::transaction::record_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record puts{};
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
    table::txs::slab_coinbase txs{};
    if (!store_.txs.get(to_txs_link(link), txs))
        return {};

    return txs.coinbase_fk;
}

// static/private
TEMPLATE
size_t CLASS::nested_count(const auto& outer) NOEXCEPT
{
    return std::accumulate(outer.begin(), outer.end(), zero,
        [](size_t total, const auto& inner) NOEXCEPT
        {
            return total + inner.size();
        });
};

TEMPLATE
input_links CLASS::to_non_coinbase_inputs(
    const header_link& link) const NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.empty())
        return {};

    std::vector<input_links> inputs(txs.size());
    std_transform(bc::par_unseq, std::next(txs.begin()), txs.end(),
        inputs.begin(), [&](const auto& tx) NOEXCEPT
        {
            return to_tx_inputs(tx);
        });

    input_links ins{};
    ins.reserve(nested_count(inputs));
    for (const auto& set: inputs)
        for (const auto& input: set)
            ins.push_back(input);

    return ins;
}

TEMPLATE
output_links CLASS::to_block_outputs(const header_link& link) const NOEXCEPT
{
    const auto txs = to_txs(link);
    output_links outs{};

    for (const auto& tx: txs)
    {
        const auto outputs = to_tx_outputs(tx);
        outs.insert(outs.end(), outputs.begin(), outputs.end());
    }

    return outs;
}

} // namespace database
} // namespace libbitcoin

#endif
