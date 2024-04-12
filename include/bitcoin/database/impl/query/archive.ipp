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

// local
// ----------------------------------------------------------------------------

template <typename Bool>
inline bool push_bool(std_vector<Bool>& stack, const Bool& element) NOEXCEPT
{
    if (!element)
        return false;

    stack.push_back(element);
    return true;
}

template <typename Link>
inline bool push_link_value(std_vector<typename Link::integer>& stack,
    const Link& element) NOEXCEPT
{
    if (element.is_terminal())
        return false;

    stack.push_back(element.value);
    return true;
}

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
inline bool CLASS::is_malleated(const block& block) const NOEXCEPT
{
    // is_malleable_duplicate is always invalid, so never stored.
    if (!block.is_malleable())
        return false;

    // This is invoked for all new blocks, most of which will not exist (cheap).
    // TODO: determine if block is a bitwise match for a stored instance, where
    // TODO: the stored and unassociated instances share the same block hash.
    // TODO: if there is an associated instance this check need not be called.
    // TODO: if the associated instance is matched then the assumption is that
    // TODO: that instance is not malleated, but that may be later determined.
    // TODO: so only disassociated instances are compared and otherwise false.
    return true;
}

TEMPLATE
inline bool CLASS::is_malleable(const header_link& link) const NOEXCEPT
{
    table::txs::get_malleable txs{};
    return store_.txs.get(to_txs_link(link), txs) && txs.malleable;
}

TEMPLATE
inline bool CLASS::is_associated(const header_link& link) const NOEXCEPT
{
    table::txs::get_associated txs{};
    return store_.txs.get(to_txs_link(link), txs) && txs.associated;
}

TEMPLATE
bool CLASS::set(const header& header, const chain_context& ctx) NOEXCEPT
{
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const header& header, const context& ctx) NOEXCEPT
{
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const block& block, const chain_context& ctx) NOEXCEPT
{
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const block& block, const context& ctx) NOEXCEPT
{
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const hash_digest& point_hash) NOEXCEPT
{
    return !set_link(point_hash).is_terminal();
}

TEMPLATE
bool CLASS::set(const block& block) NOEXCEPT
{
    return !set_link(block).is_terminal();
}

TEMPLATE
bool CLASS::set(const transaction& tx) NOEXCEPT
{
    return !set_link(tx).is_terminal();
}

TEMPLATE
bool CLASS::populate(const input& input) const NOEXCEPT
{
    if (input.prevout || input.point().is_null())
        return true;

    // input.metadata is not populated.
    // Null point would return nullptr and be interpreted as missing.
    input.prevout = get_output(input.point());
    return !is_null(input.prevout);
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
bool CLASS::get_unassociated(association& out, header_link link) const NOEXCEPT
{
    if (is_associated(link))
        return false;

    table::header::get_check_context context{};
    if (!store_.header.get(link, context))
        return false;

    out =
    {
        link,
        context.key,
        system::chain::context
        {
            context.ctx.flags,
            context.timestamp,
            context.ctx.mtp,
            system::possible_wide_cast<size_t>(context.ctx.height)
        }
    };

    return true;
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_inputs(
    const tx_link& link) const NOEXCEPT
{
    using namespace system;
    const auto fks = to_tx_spends(link);
    if (fks.empty())
        return {};

    const auto inputs = to_shared<chain::input_cptrs>();
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
    using namespace system;
    const auto fks = to_tx_outputs(link);
    if (fks.empty())
        return {};

    const auto outputs = to_shared<chain::output_cptrs>();
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
    using namespace system;
    const auto txs = to_txs(link);
    if (txs.empty())
        return {};

    const auto transactions = to_shared<chain::transaction_cptrs>();
    transactions->reserve(txs.size());

    for (const auto& tx_fk: txs)
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
    if ((child.parent_fk != header_link::terminal) &&
        !store_.header.get(child.parent_fk, parent))
        return {};

    // In case of terminal parent, parent.key defaults to null_hash.
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
    using namespace system;
    table::transaction::only_with_sk tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::slab puts{};
    puts.spend_fks.resize(tx.ins_count);
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.puts_fk, puts))
        return {};

    const auto inputs = to_shared<chain::input_cptrs>();
    const auto outputs = to_shared<chain::output_cptrs>();
    inputs->reserve(tx.ins_count);
    outputs->reserve(tx.outs_count);

    for (const auto& fk: puts.spend_fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    for (const auto& fk: puts.out_fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    const auto ptr = to_shared<transaction>
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
    const spend_link& link) const NOEXCEPT
{
    using namespace system;
    table::input::get_ptrs in{};
    table::spend::get_input spend{};
    if (!store_.spend.get(link, spend) ||
        !store_.input.get(spend.input_fk, in))
        return {};

    // Share null point instances to reduce memory consumption.
    static const auto null_point = to_shared<const point>();

    return to_shared<input>
    (
        spend.is_null() ? null_point : to_shared<point>
        (
            get_point_key(spend.point_fk),
            spend.point_index
        ),
        in.script,
        in.witness,
        spend.sequence
    );
}

TEMPLATE
typename CLASS::point::cptr CLASS::get_point(
    const spend_link& link) const NOEXCEPT
{
    table::spend::get_prevout spend{};
    if (!store_.spend.get(link, spend))
        return {};

    return system::to_shared<point>
    (
        get_point_key(spend.point_fk),
        spend.point_index
    );
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(
    const output_link& link) const NOEXCEPT
{
    using namespace system;
    const auto spend_fks = to_spenders(link);
    const auto spenders = to_shared<chain::input_cptrs>();
    spenders->reserve(spend_fks.size());

    for (const auto& spend_fk: spend_fks)
        if (!push_bool(*spenders, get_input(spend_fk)))
            return {};

    return spenders;
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
    return get_input(to_spend(link, input_index));
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    return get_spenders(to_output(link, output_index));
}

TEMPLATE
inline point_link CLASS::set_link_(const hash_digest& point_hash) NOEXCEPT
{
    if (point_hash == system::null_hash)
        return {};

    // Reuse if archived (always - this is a compression, not a guard).
    auto point_fk = to_point(point_hash);
    if (!point_fk.is_terminal())
        return point_fk;

    // This write is NOT transacted as it is only called from set_link(tx)
    // ========================================================================

    const table::point::record empty{};
    if (!store_.point.put_link(point_fk, point_hash, empty))
        return {};

    return point_fk;
    // ========================================================================
}

TEMPLATE
tx_link CLASS::set_link(const transaction& tx) NOEXCEPT
{
    using namespace system;
    if (tx.is_empty())
        return {};

    const auto key = tx.hash(false);

    // GUARD (tx redundancy)
    // This guard is only effective if there is a single database thread.
    auto tx_fk = to_tx(key);
    if (!tx_fk.is_terminal())
        return tx_fk;

    // Declare puts record.
    const auto& ins = *tx.inputs_ptr();
    const auto& outs = *tx.outputs_ptr();
    table::puts::slab puts{};
    puts.spend_fks.reserve(ins.size());
    puts.out_fks.reserve(outs.size());

    // Declare spends buffer.
    std_vector<foreign_point> spends{};
    spends.reserve(ins.size());

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate tx record.
    tx_fk = store_.tx.allocate(1);
    if (tx_fk.is_terminal())
        return {};

    // Allocate spend records.
    const auto count = possible_narrow_cast<spend_link::integer>(ins.size());
    auto spend_fk = store_.spend.allocate(count);
    if (spend_fk.is_terminal())
        return {};

    // Commit input records (spend records not indexed).
    for (const auto& in: ins)
    {
        // Commit input record.
        input_link input_fk{};
        if (!store_.input.put_link(input_fk, table::input::put_ref{ {}, *in }))
        {
            return {};
        }

        // Create point and accumulate spend keys.
        const auto& prevout = in->point();
        spends.push_back(table::spend::compose(set_link_(prevout.hash()),
            prevout.index()));

        // Write spend record.
        if (!store_.spend.set(spend_fk, table::spend::record
        {
            {},
            tx_fk,
            in->sequence(),
            input_fk
        }))
        {
            return {};
        }

        // Acumulate input (spend) in order.
        puts.spend_fks.push_back(spend_fk.value++);
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

        // Acumulate output in order.
        puts.out_fks.push_back(output_fk);
    }

    // Commit accumulated puts.
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

    // Commit spends to search.
    for (const auto& spend: views_reverse(spends))
    {
        --spend_fk.value;
        if (store_.spend.commit_link(spend_fk, spend).is_terminal())
            return {};
    }

    // Commit tx to search.
    return store_.tx.commit_link(tx_fk, key);
    // ========================================================================
}

TEMPLATE
header_link CLASS::set_link(const block& block,
    const chain_context& ctx) NOEXCEPT
{
    // Map chain context into database context.
    return set_link(block, context
    {
        system::possible_narrow_cast<context::flag::integer>(ctx.flags),
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
        system::possible_narrow_cast<context::flag::integer>(ctx.flags),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
        ctx.median_time_past
    });
}

TEMPLATE
header_link CLASS::set_link(const header& header, const context& ctx) NOEXCEPT
{
    const auto key = header.hash();

    // GUARD (header redundancy)
    // This guard is only effective if there is a single database thread.
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
header_link CLASS::set_link(const block& block, const context& ctx) NOEXCEPT
{
    const auto header_fk = set_link(block.header(), ctx);

    // Returns txs::link so translate to header::link.
    if (set_link(*block.transactions_ptr(), header_fk).is_terminal())
        return {};

    return header_fk;
}

TEMPLATE
header_link CLASS::set_link(const block& block) NOEXCEPT
{
    // This sets only the txs of a block with header/context already archived.
    const auto header_fk = to_header(block.hash());
    if (header_fk.is_terminal())
        return {};

    // Returns txs::link so translate to header::link.
    if (set_link(*block.transactions_ptr(), header_fk).is_terminal())
        return {};

    return header_fk;
}

TEMPLATE
txs_link CLASS::set_link(const transactions& txs,
    const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return{};

    // GUARDED (block (txs) redundancy)
    // This guard is only effective if there is a single database thread.
    const auto txs_link = to_txs_link(link);
    if (!txs_link.is_terminal())
        return txs_link;

    tx_links links{};
    links.reserve(txs.size());
    for (const auto& tx: txs)
        if (!push_link_value(links, set_link(*tx)))
            return {};

    const auto malleable = block::is_malleable64(txs);

    // ========================================================================
    const auto scope = store_.get_transactor();

    return store_.txs.put_link(link, table::txs::slab{ {}, malleable, links });
    // ========================================================================
}

TEMPLATE
bool CLASS::dissasociate(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    const auto malleable = is_malleable(link);

    // ========================================================================
    const auto scope = store_.get_transactor();

    return store_.txs.put_link(link, table::txs::slab{ {}, malleable, {} });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
