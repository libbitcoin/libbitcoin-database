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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Archival (mostly natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_header(const hash_digest& key) const NOEXCEPT
{
    return store_.header.exists(key);
}

TEMPLATE
inline bool CLASS::is_block(const hash_digest& key) const NOEXCEPT
{
    return is_associated(to_header(key));
}

TEMPLATE
inline bool CLASS::is_tx(const hash_digest& key) const NOEXCEPT
{
    return store_.tx.exists(key);
}

TEMPLATE
inline bool CLASS::is_coinbase(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_coinbase tx{};
    return store_.tx.get(link, tx) && tx.coinbase;
}

TEMPLATE
inline bool CLASS::is_associated(const header_link& link) const NOEXCEPT
{
    return !link.is_terminal() && store_.txs.exists(link);
}

TEMPLATE
inline bool CLASS::set(const header& header, const chain_context& ctx) NOEXCEPT
{
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const header& header, const context& ctx) NOEXCEPT
{
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const block& block, const chain_context& ctx) NOEXCEPT
{
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const block& block, const context& ctx) NOEXCEPT
{
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const hash_digest& point_hash) NOEXCEPT
{
    return !set_link(point_hash).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const transaction& tx) NOEXCEPT
{
    return !set_link(tx).is_terminal();
}

TEMPLATE
inline bool CLASS::populate(const input& input) const NOEXCEPT
{
    if (input.prevout || input.point().is_null())
        return true;

    // input.metadata is not populated.
    // Null point would return nullptr and be interpreted as missing.
    input.prevout = get_output(input.point());
    return input.prevout != nullptr;
}

TEMPLATE
bool CLASS::populate(const transaction& tx) const NOEXCEPT
{
    auto result = true;
    const auto& ins = *tx.inputs_ptr();
    std::for_each(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const block& block) const NOEXCEPT
{
    auto result = true;
    const auto ins = block.inputs_ptr();
    std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

// Archival (surrogate-keyed).
// ----------------------------------------------------------------------------

template <typename Element>
inline bool push_bool(std_vector<Element>& stack,
    const Element& element) NOEXCEPT
{
    if (!element)
        return false;

    stack.push_back(element);
    return true;
}

TEMPLATE
hashes CLASS::get_tx_keys(const header_link& link) const NOEXCEPT
{
    const auto tx_fks = to_txs(link);
    if (tx_fks.empty())
        return {};

    system::hashes hashes{};
    hashes.reserve(tx_fks.size());
    for (const auto& tx_fk: tx_fks)
        hashes.push_back(get_tx_key(tx_fk));

    // Return of any null_hash implies failure.
    return hashes;
}

TEMPLATE
inline hash_digest CLASS::get_header_key(const header_link& link) const NOEXCEPT
{
    return store_.header.get_key(link);
}

TEMPLATE
inline hash_digest CLASS::get_point_key(const point_link& link) const NOEXCEPT
{
    return store_.point.get_key(link);
}

TEMPLATE
inline hash_digest CLASS::get_tx_key(const tx_link& link) const NOEXCEPT
{
    return store_.tx.get_key(link);
}

TEMPLATE
bool CLASS::get_height(size_t& out, const header_link& link) const NOEXCEPT
{
    const auto height = get_height(link);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

TEMPLATE
bool CLASS::get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT
{
    // to_block is strong but not necessarily confirmed.
    const auto fk = to_block(link);
    return is_confirmed_block(fk) && get_height(out, fk);
}

TEMPLATE
bool CLASS::get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT
{
    // to_block is strong but not necessarily confirmed.
    const auto block_fk = to_block(link);
    if (!is_confirmed_block(block_fk))
        return false;

    // False return below implies an integrity error (tx should be indexed).
    table::txs::get_position txs{ {}, link };
    if (!store_.txs.get(to_txs_link(block_fk), txs))
        return false;

    out = txs.position;
    return true;
}

TEMPLATE
bool CLASS::get_value(uint64_t& out, const output_link& link) const NOEXCEPT
{
    table::output::get_value output{};
    if (!store_.output.get(link, output))
        return false;

    out = output.value;
    return true;
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_inputs(
    const tx_link& link) const NOEXCEPT
{
    const auto fks = to_tx_inputs(link);
    if (fks.empty())
        return {};

    const auto inputs = system::to_shared<system::chain::input_cptrs>();
    inputs->reserve(fks.size());

    for (const auto& fk: fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    return inputs;
}

TEMPLATE
typename CLASS::outputs_ptr CLASS::get_outputs(
    const tx_link& link) const NOEXCEPT
{
    const auto fks = to_tx_outputs(link);
    if (fks.empty())
        return {};

    const auto outputs = system::to_shared<system::chain::output_cptrs>();
    outputs->reserve(fks.size());

    for (const auto& fk: fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    return outputs;
}

TEMPLATE
typename CLASS::transactions_ptr CLASS::get_transactions(
    const header_link& link) const NOEXCEPT
{
    const auto fk = to_txs_link(link);
    if (fk.is_terminal())
        return {};

    table::txs::slab txs{};
    if (!store_.txs.get(fk, txs))
        return {};

    using namespace system;
    const auto transactions = to_shared<system::chain::transaction_cptrs>();
    transactions->reserve(txs.tx_fks.size());

    for (const auto& tx_fk: txs.tx_fks)
        if (!push_bool(*transactions, get_transaction(tx_fk)))
            return {};

    return transactions;
}

TEMPLATE
typename CLASS::header::cptr CLASS::get_header(
    const header_link& link) const NOEXCEPT
{
    table::header::record_with_sk child{};
    if (!store_.header.get(link, child))
        return {};

    // Terminal parent implies genesis (no parent header).
    table::header::record_sk parent{};
    if ((child.parent_fk != table::header::link::terminal) &&
        !store_.header.get(child.parent_fk, parent))
        return {};

    // parent.key lookup precludes header::record construction. 
    const auto ptr = system::to_shared<header>
    (
        child.version,
        std::move(parent.key),
        std::move(child.merkle_root),
        child.timestamp,
        child.bits,
        child.nonce
    );

    ptr->set_hash(std::move(child.key));
    return ptr;
}

TEMPLATE
typename CLASS::block::cptr CLASS::get_block(
    const header_link& link) const NOEXCEPT
{
    const auto header = get_header(link);
    if (!header)
        return {};

    const auto transactions = get_transactions(link);
    if (!transactions)
        return {};

    return system::to_shared<block>
    (
        header,
        transactions
    );
}

TEMPLATE
typename CLASS::transaction::cptr CLASS::get_transaction(
    const tx_link& link) const NOEXCEPT
{
    table::transaction::only_with_sk tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record puts{};
    puts.in_fks.resize(tx.ins_count);
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    const auto inputs = system::to_shared<system::chain::input_cptrs>();
    const auto outputs = system::to_shared<system::chain::output_cptrs>();
    inputs->reserve(tx.ins_count);
    outputs->reserve(tx.outs_count);

    for (const auto& fk: puts.in_fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    for (const auto& fk: puts.out_fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    const auto ptr = system::to_shared<transaction>
    (
        tx.version,
        inputs,
        outputs,
        tx.locktime
    );

    ptr->set_hash(std::move(tx.key));
    return ptr;
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(
    const output_link& link) const NOEXCEPT
{
    table::output::only out{};
    if (!store_.output.get(link, out))
        return {};

    return out.output;
}

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(
    const input_link& link) const NOEXCEPT
{
    table::input::only_with_prevout in{};
    if (!store_.input.get(link, in))
        return {};

    // Share null point instances to reduce memory consumption.
    static const auto null_point = system::to_shared<const point>();

    return system::to_shared<input>
    (
        in.is_null() ? null_point : system::to_shared<point>
        (
            get_point_key(in.point_fk),
            in.point_index
        ),
        in.script,
        in.witness,
        in.sequence
    );
}

TEMPLATE
typename CLASS::point::cptr CLASS::get_point(
    const input_link& link) const NOEXCEPT
{
    table::input::get_prevout in{};
    if (!store_.input.get(link, in))
        return {};

    return system::to_shared<point>
    (
        get_point_key(in.point_fk),
        in.point_index
    );
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(
    const point& prevout) const NOEXCEPT
{
    return get_output(to_tx(prevout.hash()), prevout.index());
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    return get_output(to_output(link, output_index));
}

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const tx_link& link,
    uint32_t input_index) const NOEXCEPT
{
    return get_input(to_input(link, input_index));
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(
    const output_link& link) const NOEXCEPT
{
    const auto input_fks = to_spenders(link);
    const auto spenders = system::to_shared<system::chain::input_cptrs>();
    spenders->reserve(input_fks.size());

    for (const auto& input_fk: input_fks)
        if (!push_bool(*spenders, get_input(input_fk)))
            return {};

    return spenders;
}

TEMPLATE
tx_link CLASS::set_link(const transaction& tx) NOEXCEPT
{
    if (tx.is_empty())
        return {};

    // Guard against duplicates.
    const auto key = tx.hash(false);
    auto tx_fk = to_tx(key);
    if (!tx_fk.is_terminal())
        return tx_fk;

    // Declare puts record.
    const auto& ins = *tx.inputs_ptr();
    const auto& outs = *tx.outputs_ptr();
    table::puts::record puts{};
    puts.in_fks.reserve(ins.size());
    puts.out_fks.reserve(outs.size());

    // Declare points cache.
    std_vector<foreign_point> prevouts{};
    prevouts.reserve(ins.size());

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate tx record.
    tx_fk = store_.tx.allocate(1);
    if (tx_fk.is_terminal())
        return {};

    // Commit input records.
    for (const auto& in: ins)
    {
        // Create point (hash) lookup-up table entry as required.
        const auto& prevout = in->point();
        const auto point_index = prevout.index();
        const auto point_fk = set_link(prevout.hash());

        // Accumulate spend records.
        if (!point_fk.is_terminal())
            prevouts.push_back(table::spend::compose(point_fk, point_index));

        input_link input_fk{};
        if (!store_.input.put_link(input_fk, table::input::put_ref
        {
            {},
            point_fk,
            point_index,
            tx_fk,
            *in
        }))
        {
            return {};
        }

        // Acumulate inputs in order.
        puts.in_fks.push_back(input_fk);
    }

    // Commit output records.
    for (const auto& out: outs)
    {
        output_link output_fk{};
        if (!store_.output.put_link(output_fk, table::output::put_ref
        {
            {},
            tx_fk,
            *out
        }))
        {
            return {};
        }

        // Acumulate outputs in order.
        puts.out_fks.push_back(output_fk);
    }

    // Commit puts records.
    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return {};

    // Write tx record.
    using ix = linkage<schema::index>;
    if (!store_.tx.set(tx_fk, table::transaction::record_put_ref
    {
        {},
        tx,
        system::possible_narrow_cast<ix::integer>(ins.size()),
        system::possible_narrow_cast<ix::integer>(outs.size()),
        puts_fk
    }))
    {
        return {};
    }

    // Commit prevouts (spent by the tx) to search, now that tx exists.
    const table::spend::record spender{ {}, tx_fk };
    for (const auto& prevout: prevouts)
        if (!store_.spend.put(prevout, spender))
            return {};

    // Commit tx to search.
    return store_.tx.commit_link(tx_fk, key);
    // ========================================================================
}

TEMPLATE
inline point_link CLASS::set_link(const hash_digest& point_hash) NOEXCEPT
{
    if (point_hash == system::null_hash)
        return {};

    // Reuse if archived.
    auto point_fk = to_point(point_hash);
    if (!point_fk.is_terminal())
        return point_fk;

    // ========================================================================
    const auto scope = store_.get_transactor();

    const table::point::record empty{};
    if (!store_.point.put_link(point_fk, point_hash, empty))
        return {};

    return point_fk;
    // ========================================================================
}

TEMPLATE
header_link CLASS::set_link(const block& block,
    const chain_context& ctx) NOEXCEPT
{
    // Map chain context into database context.
    return set_link(block, context
    {
        system::possible_narrow_cast<context::flag::integer>(ctx.forks),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
        ctx.median_time_past
    });
}

TEMPLATE
header_link CLASS::set_link(const header& header,
    const chain_context& ctx) NOEXCEPT
{
    // Map chain context into database context.
    return set_link(header, context
    {
        system::possible_narrow_cast<context::flag::integer>(ctx.forks),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
        ctx.median_time_past
    });
}

TEMPLATE
header_link CLASS::set_link(const block& block, const context& ctx) NOEXCEPT
{
    const auto header_fk = set_link(block.header(), ctx);
    if (header_fk.is_terminal())
        return {};

    if (is_associated(header_fk))
        return header_fk;

    tx_links links{};
    links.reserve(block.transactions_ptr()->size());
    for (const auto& tx: *block.transactions_ptr())
        links.push_back(set_link(*tx));

    return set(header_fk, links) ? header_fk : table::header::link{};
}

TEMPLATE
header_link CLASS::set_link(const header& header, const context& ctx) NOEXCEPT
{
    // This hash computation should be cached by the message deserializer.
    const auto key = header.hash();
    auto header_fk = to_header(key);
    if (!header_fk.is_terminal())
        return header_fk;

    // Parent must be missing iff its hash is null.
    const auto& parent_sk = header.previous_block_hash();
    const auto parent_fk = to_header(parent_sk);
    if (parent_fk.is_terminal() != (parent_sk == system::null_hash))
        return {};

    // ========================================================================
    const auto scope = store_.get_transactor();

    return store_.header.put_link(key, table::header::record_put_ref
    {
        {},
        ctx,
        parent_fk,
        header
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set(const header_link& link, const hashes& hashes) NOEXCEPT
{
    if (is_associated(link))
        return true;

    tx_links links{};
    links.reserve(hashes.size());
    for (const auto& hash: hashes)
        links.push_back(to_tx(hash));

    return set(link, links);
}

TEMPLATE
bool CLASS::set(const header_link& link, const tx_links& links) NOEXCEPT
{
    if (is_associated(link))
        return true;

    // This could be avoided by making this protected and guarding in callers.
    if (system::contains(links, txs_link::terminal))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    return store_.txs.put(link, table::txs::slab{ {}, links });
    // ========================================================================
}

////// TEMP: table conversion utility, by block.
////bool set_spends(const tx_link& link) NOEXCEPT;
////bool set_spends(const header_link& link) NOEXCEPT;
////bool set_spend(const foreign_point& point, const tx_link& link) NOEXCEPT;
////TEMPLATE
////bool CLASS::set_spends(const header_link& link) NOEXCEPT
////{
////    const auto txs = to_txs(link);
////    if (txs.empty())
////        return false;
////
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    return std::all_of(std::next(txs.begin()), txs.end(),
////        [&](const tx_link& tx) NOEXCEPT
////        {
////            const auto ins = to_tx_inputs(tx);
////            const table::spend::record transaction{ {}, tx };
////
////            return std::all_of(ins.begin(), ins.end(),
////                [&](const input_link& in) NOEXCEPT
////                {
////                    return store_.spend.put(to_spend_key(in), transaction);
////                });
////        });
////    // ========================================================================
////}
////
////// TEMP: table conversion utility, by tx.
////TEMPLATE
////bool CLASS::set_spends(const tx_link& link) NOEXCEPT
////{
////    // No internal non-coinbase guard.
////    const auto ins = to_tx_inputs(link);
////    const table::spend::record transaction{ {}, link };
////
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    return std::all_of(ins.begin(), ins.end(), [&](const input_link& in) NOEXCEPT
////    {
////        return store_.spend.put(to_spend_key(in), transaction);
////    });
////    // ========================================================================
////}
////
////// TEMP: table conversion utility, by tx.
////TEMPLATE
////bool CLASS::set_spend(const foreign_point& point, const tx_link& link) NOEXCEPT
////{
////    // No redundancy check, handled in set(tx), and spend is a multimap.
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    return store_.spend.put(point, table::spend::record{ {}, link });
////    // ========================================================================
////}

} // namespace database
} // namespace libbitcoin

#endif
