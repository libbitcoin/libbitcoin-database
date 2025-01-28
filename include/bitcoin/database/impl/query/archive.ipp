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
inline bool CLASS::is_milestone(const header_link& link) const NOEXCEPT
{
    table::header::get_milestone header{};
    return store_.header.get(link, header) && header.milestone;
}

TEMPLATE
inline bool CLASS::is_associated(const header_link& link) const NOEXCEPT
{
    table::txs::get_associated txs{};
    return store_.txs.find(link, txs) && txs.associated;
}

TEMPLATE
bool CLASS::set(const header& header, const chain_context& ctx,
    bool milestone) NOEXCEPT
{
    return !set_code(header, ctx, milestone);
}

TEMPLATE
bool CLASS::set(const header& header, const context& ctx, bool milestone) NOEXCEPT
{
    return !set_code(header, ctx, milestone);
}

TEMPLATE
bool CLASS::set(const block& block, const chain_context& ctx, bool milestone,
    bool strong) NOEXCEPT
{
    return !set_code(block, ctx, milestone, strong);
}

TEMPLATE
bool CLASS::set(const block& block, const context& ctx, bool milestone,
    bool strong) NOEXCEPT
{
    return !set_code(block, ctx, milestone, strong);
}

TEMPLATE
bool CLASS::set(const transaction& tx) NOEXCEPT
{
    return !set_code(tx);
}

TEMPLATE
bool CLASS::set(const block& block, bool strong) NOEXCEPT
{
    // This sets only the txs of a block with header/context already archived.
    return !set_code(block, strong);
}

// populate (with node metadata)

TEMPLATE
bool CLASS::populate(const input& input) const NOEXCEPT
{
    // Null point would return nullptr and be interpreted as missing.
    BC_ASSERT(!input.point().is_null());

    if (input.prevout)
        return true;

    const auto tx = to_tx(input.point().hash());
    input.metadata.parent = tx;
    input.metadata.inside = false;
    input.metadata.coinbase = is_coinbase(tx);
    input.prevout = get_output(tx, input.point().index());
    return !is_null(input.prevout);
}

TEMPLATE
bool CLASS::populate(const transaction& tx) const NOEXCEPT
{
    BC_ASSERT(!tx.is_coinbase());

    auto result = true;
    const auto& ins = tx.inputs_ptr();
    std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const block& block) const NOEXCEPT
{
    const auto& txs = block.transactions_ptr();
    if (txs->empty())
        return false;

    auto result = true;
    std::for_each(std::next(txs->begin()), txs->end(),
        [&](const auto& tx) NOEXCEPT
        {
            const auto& ins = tx->inputs_ptr();
            std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
            {
                result &= populate(*in);
            });
        });

    return result;
}

// populate_without_metadata

TEMPLATE
bool CLASS::populate_without_metadata(const input& input) const NOEXCEPT
{
    // Null point would return nullptr and be interpreted as missing.
    BC_ASSERT(!input.point().is_null());

    if (input.prevout)
        return true;

    const auto tx = to_tx(input.point().hash());
    input.prevout = get_output(tx, input.point().index());
    return !is_null(input.prevout);
}

TEMPLATE
bool CLASS::populate_without_metadata(const transaction& tx) const NOEXCEPT
{
    BC_ASSERT(!tx.is_coinbase());

    auto result = true;
    const auto& ins = tx.inputs_ptr();
    std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate_without_metadata(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate_without_metadata(const block& block) const NOEXCEPT
{
    const auto& txs = block.transactions_ptr();
    if (txs->empty())
        return false;

    auto result = true;
    std::for_each(std::next(txs->begin()), txs->end(),
        [&](const auto& tx) NOEXCEPT
        {
            const auto& ins = tx->inputs_ptr();
            std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
            {
                result &= populate_without_metadata(*in);
            });
        });

    return result;
}

// Archival (surrogate-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
hashes CLASS::get_tx_keys(const header_link& link) const NOEXCEPT
{
    const auto tx_fks = to_transactions(link);
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
size_t CLASS::get_tx_count(const header_link& link) const NOEXCEPT
{
    table::txs::get_tx_quantity txs{};
    if (!store_.txs.find(link, txs))
        return {};

    return txs.quantity;
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
bool CLASS::get_height(size_t& out, const hash_digest& key) const NOEXCEPT
{
    const auto height = get_height(key);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

TEMPLATE
bool CLASS::get_height(size_t& out, const header_link& link) const NOEXCEPT
{
    // Use get_height(..., key) in place of get(to_header(key)).
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
    const auto fk = to_block(link);
    if (!is_confirmed_block(fk))
        return false;

    // False return below implies an integrity error (tx should be indexed).
    table::txs::get_position txs{ {}, link };
    if (!store_.txs.find(fk, txs))
        return false;

    out = txs.position;
    return true;
}

TEMPLATE
bool CLASS::get_tx_sizes(size_t& light, size_t& heavy,
    const tx_link& link) const NOEXCEPT
{
    table::transaction::get_sizes sizes{};
    if (!store_.tx.get(link, sizes))
        return false;

    light = sizes.light;
    heavy = sizes.heavy;
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
bool CLASS::get_unassociated(association& out, const header_link& link) const NOEXCEPT
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
    // TODO: eliminate shared memory pointer reallocations.
    using namespace system;
    const auto fks = to_spends(link);
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
    // TODO: eliminate shared memory pointer reallocations.
    using namespace system;
    const auto fks = to_outputs(link);
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
    // TODO: eliminate shared memory pointer reallocations.
    using namespace system;
    const auto txs = to_transactions(link);
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
size_t CLASS::get_candidate_size() const NOEXCEPT
{
    // If the store is not opened this will be a max_size loop.
    return get_candidate_size(get_top_candidate());
}

TEMPLATE
size_t CLASS::get_candidate_size(size_t top) const NOEXCEPT
{
    size_t wire{};
    for (auto height = zero; height <= top; ++height)
        wire += get_block_size(to_candidate(height));

    return wire;
}

TEMPLATE
size_t CLASS::get_confirmed_size() const NOEXCEPT
{
    // If the store is not opened this will be a max_size loop.
    return get_confirmed_size(get_top_confirmed());
}

TEMPLATE
size_t CLASS::get_confirmed_size(size_t top) const NOEXCEPT
{
    size_t wire{};
    for (auto height = zero; height <= top; ++height)
        wire += get_block_size(to_confirmed(height));

    return wire;
}

TEMPLATE
size_t CLASS::get_block_size(const header_link& link) const NOEXCEPT
{
    table::txs::get_block_size txs{};
    return store_.txs.find(link, txs) ? txs.wire : zero;
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
    // TODO: eliminate shared memory pointer reallocations.
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

    // Witness hash is not retained by the store.
    ptr->set_nominal_hash(std::move(tx.key));
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

    // TODO: eliminate shared memory pointer reallocation.
    for (const auto& spend_fk: spend_fks)
        if (!push_bool(*spenders, get_input(spend_fk)))
            return {};

    return spenders;
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

// set transaction
// ----------------------------------------------------------------------------
// The only multitable write query (except initialize/genesis).

TEMPLATE
code CLASS::set_code(const transaction& tx) NOEXCEPT
{
    tx_link unused{};
    return set_code(unused, tx);
}

TEMPLATE
code CLASS::set_code(tx_link& out_fk, const transaction& tx) NOEXCEPT
{
    using namespace system;
    if (tx.is_empty())
        return error::tx_empty;

    // tx.get_hash() assumes cached or is not thread safe.
    const auto& key = tx.get_hash(false);

    // Declare puts record.
    const auto& ins = tx.inputs_ptr();
    const auto& outs = tx.outputs_ptr();
    table::puts::slab puts{};
    puts.spend_fks.reserve(ins->size());
    puts.out_fks.reserve(outs->size());

    // Declare spends buffer.
    std::vector<foreign_point> spends{};
    spends.reserve(ins->size());

    // TODO: eliminate shared memory pointer reallocations.
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate tx record.
    // Clean single allocation failure (e.g. disk full).
    out_fk = store_.tx.allocate(1);
    if (out_fk.is_terminal())
        return error::tx_tx_allocate;

    // Allocate spend records.
    // Clean single allocation failure (e.g. disk full).
    const auto count = possible_narrow_cast<spend_link::integer>(ins->size());
    auto spend_fk = store_.spend.allocate(count);
    if (spend_fk.is_terminal())
        return error::tx_spend_allocate;

    // Commit input records (spend records not indexed).
    for (const auto& in: *ins)
    {
        // Commit input record.
        // Safe allocation failure, blob linked by unindexed spend.
        input_link input_fk{};
        if (!store_.input.put_link(input_fk, table::input::put_ref
        {
            {},
            *in
        }))
        {
            return error::tx_input_put;
        }

        // Input point aliases.
        const auto& prevout = in->point();
        const auto& hash = prevout.hash();

        // Create prevout hash in point table.
        point_link hash_fk{};
        if (hash != null_hash)
        {
            // GUARD (tx redundancy)
            // Only fully effective if there is a single database thread.
            // This reduces point store by ~45GiB, but causes thrashing.
            if (minimize_)
                hash_fk = to_point(hash);

            if (hash_fk.is_terminal())
            {
                // Safe allocation failure, duplicates limited but expected.
                if (!store_.point.put_link(hash_fk, hash, table::point::record
                {
                    // Table stores no data other than the search key.
                }))
                {
                    return error::tx_point_put;
                }
            }
        }

        // Accumulate spend keys in order (terminal for any null point).
        spends.push_back(table::spend::compose(hash_fk, prevout.index()));

        // Write spend record.
        // Safe allocation failure, index is deferred because invalid tx_fk.
        if (!store_.spend.set(spend_fk, table::spend::record
        {
            {},
            out_fk,
            in->sequence(),
            input_fk
        }))
        {
            return error::tx_spend_set;
        }

        // Accumulate spends (input references) in order.
        puts.spend_fks.push_back(spend_fk.value++);
    }

    // Commit output records.
    for (const auto& out: *outs)
    {
        // Safe allocation failure, blob unlinked.
        output_link output_fk{};
        if (!store_.output.put_link(output_fk, table::output::put_ref
        {
            {},
            out_fk,
            *out
        }))
        {
            return error::tx_output_put;
        }

        // Accumulate outputs in order.
        puts.out_fks.push_back(output_fk);
    }

    // Commit accumulated puts.
    // Safe allocation failure, unlinked blob links spend/output blobs.
    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return error::tx_puts_put;

    // Write tx record.
    // Safe allocation failure, index is deferred for spend index consistency.
    using ix = linkage<schema::index>;
    if (!store_.tx.set(out_fk, table::transaction::record_put_ref
    {
        {},
        tx,
        system::possible_narrow_cast<ix::integer>(ins->size()),
        system::possible_narrow_cast<ix::integer>(outs->size()),
        puts_fk
    }))
    {
        return error::tx_tx_set;
    }

    // Commit spends to search.
    // Safe allocation failure, unindexed txs linked by spend, others unlinked.
    // A replay of committed spends without indexed tx will appear as double
    // spends, but the spend cannot be confirmed without the indexed tx. Spends
    // without indexed txs should be suppressed by c/s interface query.
    for (const auto& spend: views_reverse(spends))
    {
        --spend_fk.value;
        if (store_.spend.commit_link(spend_fk, spend).is_terminal())
            return error::tx_spend_commit;
    }

    // Commit addresses to search if address index is enabled.
    if (address_enabled())
    {
        auto output_fk = puts.out_fks.begin();
        for (const auto& out: *outs)
        {
            // Safe allocation failure, unindexed tx outputs linked by address,
            // others unlinked. A replay of committed addresses without indexed
            // tx will appear as double spends, but the spend cannot be
            // confirmed without the indexed tx. Addresses without indexed txs
            // should be suppressed by c/s interface query.
            if (!store_.address.put(out->script().hash(), table::address::record
            {
                {},
                *output_fk++
            }))
            {
                return error::tx_address_put;
            }
        }
    }

    // Commit tx to search.
    // Clean single allocation failure (e.g. disk full).
    return store_.tx.commit(out_fk, key) ? error::success : error::tx_tx_commit;
    // ========================================================================
}

// set header
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::set_code(const header& header, const context& ctx,
    bool milestone) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, header, ctx, milestone);
}

TEMPLATE
code CLASS::set_code(const header& header, const chain_context& ctx,
    bool milestone) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, header, ctx, milestone);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const header& header,
    const chain_context& ctx, bool milestone, bool) NOEXCEPT
{
    // Map chain context into database context.
    return set_code(out_fk, header, context
    {
        system::possible_narrow_cast<context::flag::integer>(ctx.flags),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
        ctx.median_time_past
    }, milestone);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const header& header,
    const context& ctx, bool milestone, bool) NOEXCEPT
{
    // header.get_hash() assumes cached or is not thread safe.
    const auto& key = header.get_hash();

    ////// GUARD (header redundancy)
    ////// This is only fully effective if there is a single database thread.
    ////out_fk = to_header(key);
    ////if (!out_fk.is_terminal())
    ////    return error::success;

    // Parent must be missing iff its hash is null.
    const auto& previous = header.previous_block_hash();
    const auto parent_fk = to_header(previous);
    if (parent_fk.is_terminal() != (previous == system::null_hash))
        return system::error::orphan_block;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    out_fk = store_.header.put_link(key, table::header::record_put_ref
    {
        {},
        ctx,
        milestone,
        parent_fk,
        header
    });

    return out_fk.is_terminal() ? error::header_put : error::success;
    // ========================================================================
}

// set block
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::set_code(const block& block, const context& ctx, bool milestone,
    bool strong) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, ctx, milestone, strong);
}

TEMPLATE
code CLASS::set_code(const block& block, const chain_context& ctx,
    bool milestone, bool strong) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, ctx, milestone, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    const chain_context& ctx, bool milestone, bool strong) NOEXCEPT
{
    // Map chain context into database context.
    return set_code(out_fk, block, context
    {
        system::possible_narrow_cast<context::flag::integer>(ctx.flags),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
        ctx.median_time_past
    }, milestone, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    const context& ctx, bool milestone, bool strong) NOEXCEPT
{
    const auto ec = set_code(out_fk, block.header(), ctx, milestone);
    return ec ? ec : set_code(block, out_fk, strong);
}

// set txs from block
// ----------------------------------------------------------------------------
// This sets only the txs of a block with header/context already archived.
// Block MUST be kept in scope until all transactions are written. ~block()
// releases all memory for parts of itself, due to the custom allocator.

TEMPLATE
code CLASS::set_code(const block& block, bool strong) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    bool strong) NOEXCEPT
{
    out_fk = to_header(block.get_hash());
    if (out_fk.is_terminal())
        return error::txs_header;

    return set_code(block, out_fk, strong);
}

TEMPLATE
code CLASS::set_code(const block& block, const header_link& key,
    bool strong) NOEXCEPT
{
    txs_link unused{};
    return set_code(unused, block, key, strong);
}

TEMPLATE
code CLASS::set_code(txs_link& out_fk, const block& block,
    const header_link& key, bool strong) NOEXCEPT
{
    if (key.is_terminal())
        return error::txs_header;

    const auto count = block.transactions();
    if (is_zero(count))
        return error::txs_empty;

    ////// GUARD (block (txs) redundancy)
    ////// This is only fully effective if there is a single database thread.
    ////out_fk = to_txs(key);
    ////if (!out_fk.is_terminal())
    ////    return error::success;

    code ec{};
    tx_link tx_fk{};
    tx_links links{};
    links.reserve(count);
    for (const auto& tx: *block.transactions_ptr())
    {
        // Each tx is set under a distinct transactor.
        if ((ec = set_code(tx_fk, *tx)))
            return ec;

        links.push_back(tx_fk.value);
    }

    using bytes = linkage<schema::size>::integer;
    const auto size = block.serialized_size(true);
    const auto wire = system::possible_narrow_cast<bytes>(size);

    // ========================================================================
    const auto scope = store_.get_transactor();
    constexpr auto positive = true;

    // Clean allocation failure (e.g. disk full), see set_strong() comments.
    // Transactor assures cannot be restored without txs, as required to unset.
    if (strong && !set_strong(key, links, positive))
        return error::txs_confirm;

    // Header link is the key for the txs table.
    // Clean single allocation failure (e.g. disk full).
    out_fk = store_.txs.put_link(key, table::txs::slab
    {
        {},
        wire,
        links
    });

    return out_fk.is_terminal() ? error::txs_txs_put : error::success;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
