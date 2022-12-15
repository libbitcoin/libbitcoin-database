/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_IPP
#define LIBBITCOIN_DATABASE_QUERY_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

// Error handling:
// Negative (false/empty/nullptr) return implies error or not found condition.
// Interpretation may depend on context. The presumption is of store integrity,
// as there is no validation (such as checksumming) of the store. Given that
// the blockchain is a cache of public data, this is a sufficient loss guard.
// If a validated link is passed and the result is negative, the integrity
// presumption implies that the result is negative - unless there are no such
// conditions. In that case the failure result implies loss of store integrity.
// Given the integrity presumption, such a possibility can be ignored. On the
// other hand, if a possible negative result from one call is cascaded to
// another call, the presumption on a final negative is that a negative result
// was propagated, or that a positive propagated, follewed by a negative. In
// this case the original cause of the negative is presumed to be unimportant.
// Store integrity is no assured if the indexes are empty (no genesis block),
// and therefore assertions are provided for these limited situations, as
// error handling would be unnecessarily costly. The integrity presumption
// does not hold given faults in the underlying store interface. Consequently
// these are managed independently through the logging interface. Note that
// cascading failures from terminal to search key is an unnecessary perf hit.
// Note that expected stream invalidation may occur (index validation).
//
// Remapping:
// During any write operation, the store may be forced to unload and remap the
// underlying backing file of one or more table bodies (headers do not resize).
// This invalidates all memory pointers, but does not invalidate links.
// Pointers are short-lived and managed internally by tables. However iterators
// encapsulate pointers and therefore should be be used with caution. Holding
// an iterator on a table while also invoking a write on that table will result
// in deadlock in the case where the write invokes a remap (blocked by the
// iterator).
//
// Transactionality:
// The query interface uses the underlying store's transactor to provide an
// assurance that writes are consistent when the transactor is unlocked. This
// is provided so that store-wide archival may proceed with writes suspended at
// a point of consistency, and allowing reads to proceed. Query callers should
// expect writes to be blocked during store hot backup.

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_USE_OF_MOVED_OBJECT)

TEMPLATE
CLASS::query(Store& value) NOEXCEPT
  : store_(value)
{
}

// Initialization (natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_empty() NOEXCEPT
{
    // True return implies genesis not indexed.
    return is_zero(store_.confirmed.count()) ||
        is_zero(store_.candidate.count());
}

TEMPLATE
size_t CLASS::get_top() NOEXCEPT
{
    BC_ASSERT_MSG(!is_zero(store_.confirmed.count()), "empty");
    return sub1(store_.confirmed.count());
}

TEMPLATE
size_t CLASS::get_top_candidate() NOEXCEPT
{
    BC_ASSERT_MSG(!is_zero(store_.candidate.count()), "empty");
    return sub1(store_.candidate.count());
}

TEMPLATE
size_t CLASS::get_fork() NOEXCEPT
{
    for (auto height = get_top(); !is_zero(height); --height)
        if (to_confirmed(height) == to_candidate(height))
            return height;

    // Should not be called during organization.
    return zero;
}

TEMPLATE
size_t CLASS::get_last_associated_from(size_t height) NOEXCEPT
{
    if (height == max_size_t)
        return height;

    // Should not be called during organization.
    while (is_associated(to_candidate(++height)));
    return --height;
}

TEMPLATE
hashes CLASS::get_all_unassociated_above(size_t height) NOEXCEPT
{
    hashes out{};
    const auto top = get_top_candidate();
    while (height < top)
    {
        const auto header_fk = to_candidate(++height);
        if (!is_associated(header_fk))
            out.push_back(store_.header.get_key(header_fk));
    }

    // Should not be called during organization.
    return out;
}

TEMPLATE
hashes CLASS::get_locator(const heights& heights) NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());
    for (const auto& height: heights)
    {
        const auto header_fk = to_confirmed(height);
        if (!header_fk.is_terminal())
            out.push_back(store_.header.get_key(header_fk));
    }

    // A missing hash implies one or more unindexed heights.
    // Due to reorganization, top may decrease intermittently.
    out.shrink_to_fit();
    return out;
}

// Key conversion. 
// ----------------------------------------------------------------------------

// natural key (entry)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
header_link CLASS::to_candidate(size_t height) NOEXCEPT
{
    // Terminal return implies height above top candidate.
    return store_.candidate.first(system::possible_narrow_cast<
        table::height::block::integer>(height));
}

TEMPLATE
header_link CLASS::to_confirmed(size_t height) NOEXCEPT
{
    // Terminal return implies height above top confirmed.
    return store_.confirmed.first(system::possible_narrow_cast<
        table::height::block::integer>(height));
}

TEMPLATE
header_link CLASS::to_header(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies not archived.
    return store_.header.first(key);
}

TEMPLATE
input_links CLASS::to_spenders(const point& prevout) NOEXCEPT
{
    // Empty return implies null point.
    if (prevout.is_null())
        return {};

    // Empty return implies point/input not archived.
    const auto point_fk = to_point(prevout.hash());
    if (point_fk.is_terminal())
        return {};

    // Empty return implies input not yet committed for the point (race).
    const auto input_sk = table::input::compose(point_fk, prevout.index());
    const auto it = store_.input.it(input_sk);
    if (it.self().is_terminal())
        return {};

    input_links spenders{};
    do { spenders.push_back(it.self()); } while (it.advance());
    return spenders;
}

TEMPLATE
point_link CLASS::to_point(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies not archived.
    return store_.point.first(key);
}

TEMPLATE
txs_link CLASS::to_txs(const header_link& link) NOEXCEPT
{
    // Terminal return implies not associated (possibly header not archived).
    return store_.txs.first(link);
}

TEMPLATE
tx_link CLASS::to_tx(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies not archived.
    return store_.tx.first(key);
}

// put to tx (reverse navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
tx_link CLASS::to_input_tx(const input_link& link) NOEXCEPT
{
    // Terminal return implies invalid link.
    table::input::get_parent in{};
    if (!store_.input.get(link, in))
        return {};

    return in.parent_fk;
}

TEMPLATE
tx_link CLASS::to_output_tx(const output_link& link) NOEXCEPT
{
    // Terminal return implies invalid link.
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    return out.parent_fk;
}

TEMPLATE
tx_link CLASS::to_prevout_tx(const input_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or null point.
    table::input::slab_decomposed_fk in{};
    if (!store_.input.get(link, in) || in.is_null())
        return {};

    // Terminal return implies prevout tx not archived.
    return to_tx(store_.point.get_key(in.point_fk));
}

// point to put (forward navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
input_link CLASS::to_input(const tx_link& link, uint32_t input_index) NOEXCEPT
{
    // Terminal return implies invalid link and/or index.
    table::transaction::record_input tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    return tx.input_fk;
}

TEMPLATE
output_link CLASS::to_output(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    // Terminal return implies invalid link and/or index.
    table::transaction::record_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    return tx.output_fk;
}

TEMPLATE
output_link CLASS::to_prevout(const input_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or null point.
    table::input::slab_decomposed_sk in{};
    if (!store_.input.get(link, in) || in.is_null())
        return {};

    // Terminal return implies prevout tx not archived.
    return to_output(to_tx(store_.point.get_key(in.point_fk)), in.point_index);
}

// output to spenders (reverse navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
input_links CLASS::to_spenders(const output_link& link) NOEXCEPT
{
    // Empty return implies invalid link.
    table::output::get_point out{};
    if (!store_.output.get(link, out))
        return {};

    // Empty return implies no spenders.
    return to_spenders(out.parent_fk, out.index);
}

TEMPLATE
input_links CLASS::to_spenders(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    // Empty return implies invalid link or no spenders.
    return to_spenders(point{ store_.tx.get_key(link), output_index });
}

// block/tx to puts (forward navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
input_links CLASS::to_tx_inputs(const tx_link& link) NOEXCEPT
{
    // Empty return implies invalid link.
    table::transaction::record_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // Empty return implies store inconsistency.
    table::puts::record puts{};
    puts.in_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    return std::move(puts.in_fks);
}

TEMPLATE
output_links CLASS::to_tx_outputs(const tx_link& link) NOEXCEPT
{
    // Empty return implies invalid link.
    table::transaction::record_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // Empty return implies store inconsistency.
    table::puts::record puts{};
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.outs_fk(), puts))
        return {};

    return std::move(puts.out_fks);
}

TEMPLATE
input_links CLASS::to_block_inputs(const header_link& link) NOEXCEPT
{
    // Empty return implies invalid link or unassociated.
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    // TODO: evaluate precount of inputs for vector reservation.
    input_links ins{};
    for (const auto& tx: txs)
    {
        // Empty return implies store inconsistency.
        const auto inputs = to_tx_inputs(tx);
        if (inputs.empty())
            return {};

        ins.insert(ins.end(), inputs.begin(), inputs.end());
    }

    return ins;
}

TEMPLATE
output_links CLASS::to_block_outputs(const header_link& link) NOEXCEPT
{
    // Empty return implies invalid link or unassociated.
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    // TODO: evaluate precount of inputs for vector reservation.
    output_links outs{};
    for (const auto& tx: txs)
    {
        // Empty return implies store inconsistency.
        const auto outputs = to_tx_outputs(tx);
        if (outputs.empty())
            return {};

        outs.insert(outs.end(), outputs.begin(), outputs.end());
    }

    return outs;
}

// block to txs (forward navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
tx_links CLASS::to_transactions(const header_link& link) NOEXCEPT
{
    // Empty return implies invalid link or unassociated.
    table::txs::slab txs{};
    if (!store_.txs.get(to_txs(link), txs))
        return {};

    return std::move(txs.tx_fks);
}

// tx to blocks (reverse navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
header_link CLASS::strong_by(const tx_link& link) NOEXCEPT
{
    // Terminal return implies invalid link.
    table::strong_tx::record strong{};
    if (!store_.strong_tx.get(store_.strong_tx.first(link), strong))
        return {};

    // Terminal return implies strong block reverted.
    return strong.header_fk;
}

// Archival (natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_header(const hash_digest& key) NOEXCEPT
{
    // False return implies not archived.
    return store_.header.exists(key);
}

TEMPLATE
bool CLASS::is_block(const hash_digest& key) NOEXCEPT
{
    // False return implies not archived or not associated.
    return is_associated(to_header(key));
}

TEMPLATE
bool CLASS::is_tx(const hash_digest& key) NOEXCEPT
{
    // False return implies not archived.
    return store_.tx.exists(key);
}

TEMPLATE
bool CLASS::set(const header& header, const context& ctx) NOEXCEPT
{
    // See set_link for details.
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const block& block, const context& ctx) NOEXCEPT
{
    // See set_link for details.
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const transaction& tx) NOEXCEPT
{
    // See set_link for details.
    return !set_link(tx).is_terminal();
}

TEMPLATE
bool CLASS::populate(const input& input) NOEXCEPT
{
    // False return (and nullptr assigned) implies prevout not found.
    return ((input.prevout = get_output(input.point())));
}

TEMPLATE
bool CLASS::populate(const transaction& tx) NOEXCEPT
{
    auto result = true;
    const auto& ins = *tx.inputs_ptr();

    // False return (and nullptrs assigned) implies not all prevouts populated.
    std::for_each(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const block& block) NOEXCEPT
{
    auto result = true;
    const auto ins = block.inputs_ptr();

    // False return (and nullptrs assigned) implies not all prevouts populated.
    std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

// Archival (foreign-keyed).
// ----------------------------------------------------------------------------

template <typename Element>
inline bool push_bool(std_vector<Element>& stack, const Element& element)
{
    if (!element)
        return false;

    stack.push_back(element);
    return true;
}

TEMPLATE
hashes CLASS::get_txs(const header_link& link) NOEXCEPT
{
    // Empty return implies block not associated.
    table::txs::slab txs{};
    if (!store_.txs.get(to_txs(link), txs))
        return {};

    system::hashes hashes{};
    hashes.reserve(txs.tx_fks.size());

    // Return of any null hashes (tx.get_key) implies store inconsistency.
    for (const auto& fk: txs.tx_fks)
        hashes.push_back(store_.tx.get_key(fk));

    return hashes;
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_inputs(const tx_link& link) NOEXCEPT
{
    // NULL return implies invalid link.
    const auto fks = to_tx_inputs(link);
    if (fks.empty())
        return {};

    const auto inputs = system::to_shared<system::chain::input_cptrs>();
    inputs->reserve(fks.size());

    // nullptr return implies store inconsistency.
    for (const auto& fk: fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    return inputs;
}

TEMPLATE
typename CLASS::outputs_ptr CLASS::get_outputs(const tx_link& link) NOEXCEPT
{
    // NULL return implies invalid link.
    const auto fks = to_tx_outputs(link);
    if (fks.empty())
        return {};

    const auto outputs = system::to_shared<system::chain::output_cptrs>();
    outputs->reserve(fks.size());

    // nullptr return implies store inconsistency.
    for (const auto& fk: fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    return outputs;
}

TEMPLATE
typename CLASS::transactions_ptr CLASS::get_transactions(
    const header_link& link) NOEXCEPT
{
    // nullptr return implies invalid link or unassociated.
    table::txs::slab txs{};
    if (!store_.txs.get(to_txs(link), txs))
        return {};

    using namespace system;
    const auto transactions = to_shared<system::chain::transaction_ptrs>();
    transactions->reserve(txs.tx_fks.size());

    // nullptr return implies store inconsistency.
    for (const auto& fk: txs.tx_fks)
        if (!push_bool(*transactions, get_tx(fk)))
            return {};

    return transactions;
}

TEMPLATE
typename CLASS::header::cptr CLASS::get_header(const header_link& link) NOEXCEPT
{
    // nullptr return implies invalid link.
    table::header::record child{};
    if (!store_.header.get(link, child))
        return {};

    // nullptr return implies store inconsistency (missing parent).
    // Terminal parent implies genesis (no parent header).
    table::header::record_sk parent{};
    if ((child.parent_fk != table::header::link::terminal) &&
        !store_.header.get(child.parent_fk, parent))
        return {};

    return system::to_shared(new header
    {
        child.version,
        std::move(parent.key),
        std::move(child.merkle_root),
        child.timestamp,
        child.bits,
        child.nonce
    });
}

TEMPLATE
typename CLASS::block::cptr CLASS::get_block(const header_link& link) NOEXCEPT
{
    // nullptr return implies invalid link.
    const auto header = get_header(link);
    if (!header)
        return {};

    // nullptr return implies invalid link or unassociated.
    const auto transactions = get_transactions(link);
    if (!transactions)
        return {};

    return system::to_shared(new block
    {
        header,
        transactions
    });
}

TEMPLATE
typename CLASS::transaction::cptr CLASS::get_tx(const tx_link& link) NOEXCEPT
{
    // nullptr return implies invalid link.
    table::transaction::only tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // nullptr return implies store inconsistency.
    table::puts::record puts{};
    puts.in_fks.resize(tx.ins_count);
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    const auto inputs = system::to_shared<system::chain::input_cptrs>();
    const auto outputs = system::to_shared<system::chain::output_cptrs>();
    inputs->reserve(tx.ins_count);
    outputs->reserve(tx.outs_count);

    // nullptr return implies store inconsistency.
    for (const auto& fk: puts.in_fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    // nullptr return implies store inconsistency.
    for (const auto& fk: puts.out_fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    return system::to_shared(new transaction
    {
        tx.version,
        inputs,
        outputs,
        tx.locktime
    });
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const output_link& link) NOEXCEPT
{
    // nullptr return implies invalid link.
    table::output::only out{};
    if (!store_.output.get(link, out))
        return {};

    return out.output;
}

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const input_link& link) NOEXCEPT
{
    // nullptr return implies invalid link.
    table::input::only_with_decomposed_sk in{};
    if (!store_.input.get(link, in))
        return {};

    // Share null point instances to reduce memory consumption.
    static const auto null_point = system::to_shared<const point>();

    return system::to_shared(new input
    {
        in.is_null() ? null_point : system::to_shared(new point
        {
            store_.point.get_key(in.point_fk),
            in.point_index
        }),
        in.script,
        in.witness,
        in.sequence
    });
}

TEMPLATE
typename CLASS::point::cptr CLASS::get_point(const input_link& link) NOEXCEPT
{
    // nullptr return implies invalid link.
    table::input::slab_decomposed_sk in{};
    if (!store_.input.get(link, in))
        return {};

    return system::to_shared(new point
    {
        store_.point.get_key(in.point_fk),
        in.point_index
    });
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const point& prevout) NOEXCEPT
{
    // nullptr return implies null point.
    // Shortcircuits get_output(to_tx(null_hash)) fault.
    if (prevout.is_null())
        return {};

    // nullptr return implies tx and/or index not found (link is derived).
    return get_output(to_tx(prevout.hash()), prevout.index());
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    // nullptr return implies invalid link or tx and/or index not found.
    table::transaction::record_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    // nullptr return implies store inconsistency.
    table::puts::record_get_one puts{};
    if (!store_.puts.get(tx.output_fk, puts))
        return {};

    // nullptr return implies store inconsistency (link is derived).
    return get_output(puts.put_fk);
}

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const tx_link& link,
    uint32_t input_index) NOEXCEPT
{
    // nullptr return implies invalid link or tx and/or index not found.
    table::transaction::record_input tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    // nullptr return implies store inconsistency.
    table::puts::record_get_one puts{};
    if (!store_.puts.get(tx.input_fk, puts))
        return {};

    // nullptr return implies store inconsistency (link is derived).
    return get_input(puts.put_fk);
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    const auto fks = to_spenders(link, output_index);
    const auto spenders = system::to_shared<system::chain::input_cptrs>();
    spenders->reserve(fks.size());

    // nullptr return implies store inconsistency (links are derived).
    for (const auto& fk: fks)
        if (!push_bool(*spenders, get_input(fk)))
            return {};

    // Empty return implies no spenders.
    return spenders;
}

// protected
TEMPLATE
typename CLASS::input_key CLASS::make_foreign_point(
    const point& prevout) NOEXCEPT
{
    // Terminal foreign point return [terminal, null_index] implies null point.
    if (prevout.is_null())
        return table::input::compose({}, prevout.index());

    // Reuse point hash fk if archived.
    auto point_fk = to_point(prevout.hash());

    // Create point hash fk if not found.
    if (point_fk.is_terminal())
    {
        // Terminal return implies allocation failure.
        const table::point::record empty{};
        if (!store_.point.put_link(point_fk, prevout.hash(), empty))
            return {};
    }

    return table::input::compose(point_fk, prevout.index());
}

TEMPLATE
tx_link CLASS::set_link(const transaction& tx) NOEXCEPT
{
    // Terminal return implies no inputs and/or outputs.
    if (tx.is_empty())
        return {};

    // Save for final commit.
    const auto key = tx.hash(false);

    // Return with link if tx exists.
    auto tx_fk = to_tx(key);
    if (!tx_fk.is_terminal())
        return tx_fk;

    // Declare puts record.
    const auto& ins = *tx.inputs_ptr();
    const auto& outs = *tx.outputs_ptr();
    table::puts::record puts{};
    puts.in_fks.reserve(ins.size());
    puts.out_fks.reserve(outs.size());

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Terminal return implies allocation failure.
    tx_fk = store_.tx.allocate(1);
    if (tx_fk.is_terminal())
        return {};

    uint32_t input_index = 0;
    linkage<schema::put> put_fk{};
    for (const auto& in: ins)
    {
        // Terminal return implies allocation failure.
        if (!store_.input.set_link(put_fk, table::input::slab_put_ref
        {
            {},
            tx_fk,
            input_index++,
            *in
        }))
        {
            return {};
        }

        puts.in_fks.push_back(put_fk);
    }

    uint32_t output_index = 0;
    for (const auto& out: outs)
    {
        // Terminal return implies allocation failure.
        if (!store_.output.put_link(put_fk, table::output::slab_put_ref
        {
            {},
            tx_fk,
            output_index++,
            *out
        }))
        {
            return {};
        }

        puts.out_fks.push_back(put_fk);
    }

    // Terminal return implies allocation failure.
    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return {};

    // Terminal return implies serialization failure.
    using ix = table::transaction::ix::integer;
    if (!store_.tx.set(tx_fk, table::transaction::record_put_ref
    {
        {},
        tx,
        system::possible_narrow_cast<ix>(ins.size()),
        system::possible_narrow_cast<ix>(outs.size()),
        puts_fk
    }))
    {
        return {};
    }

    // Terminal return implies memory access failure.
    // Commit each input to its seach key (prevout foreign point).
    auto input_fk = puts.in_fks.begin();
    for (const auto& in: ins)
        if (!store_.input.commit(*input_fk++, make_foreign_point(in->point())))
            return {};

    // Terminal return implies memory access failure.
    return store_.tx.commit_link(tx_fk, key);
    // ========================================================================
}

TEMPLATE
header_link CLASS::set_link(const block& block, const context& ctx) NOEXCEPT
{
    // Terminal return implies inconsistent parent or allocation failure.
    const auto header_fk = set_link(block.header(), ctx);
    if (header_fk.is_terminal())
        return {};

    // Shortcircuit fk generation.
    if (is_associated(header_fk))
        return header_fk;

    tx_links links{};
    links.reserve(block.transactions_ptr()->size());

    // Get/create foreign key for each tx (terminal possible).
    for (const auto& tx: *block.transactions_ptr())
        links.push_back(set_link(*tx));

    // Set is idempotent, requires that none are terminal.
    // Terminal return implies allocation failure (transactions terminal).
    return set(header_fk, links) ? header_fk : table::header::link{};
}

TEMPLATE
header_link CLASS::set_link(const header& header, const context& ctx) NOEXCEPT
{
    // Parent must be missing iff its hash is null.
    // Terminal return implies inconsistent parent reference.
    const auto& parent_sk = header.previous_block_hash();
    const auto parent_fk = to_header(parent_sk);
    if (parent_fk.is_terminal() != (parent_sk == system::null_hash))
        return {};

    // Return with success if header exists.
    const auto key = header.hash();
    auto header_fk = to_header(key);
    if (!header_fk.is_terminal())
        return header_fk;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Terminal return implies allocation failure.
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
    // Shortcircuit fk generation.
    if (is_associated(link))
        return true;

    tx_links links{};
    links.reserve(hashes.size());

    // Create foreign key for each tx (terminal possible).
    for (const auto& hash: hashes)
        links.push_back(to_tx(hash));

    // False return implies allocation failure (transactions terminal).
    return set(link, links);
}

TEMPLATE
bool CLASS::set(const header_link& link, const tx_links& links) NOEXCEPT
{
    if (is_associated(link))
        return true;

    // False return implies terminal limits in collection (prior failures).
    if (system::contains(links, txs_link::terminal))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.txs.put(link, table::txs::slab{ {}, links });
    // ========================================================================
}

// Validation (foreign-keyed).
// ----------------------------------------------------------------------------

// protected
TEMPLATE
code CLASS::to_block_code(linkage<schema::code>::integer value) NOEXCEPT
{
    // validated_bk code to error code.
    switch (value)
    {
    case schema::block_state::confirmable:
        return error::block_confirmable;
    case schema::block_state::preconfirmable:
        return error::block_preconfirmable;
    case schema::block_state::unconfirmable:
        return error::block_unconfirmable;
    default:
        return error::unknown;
    }
}

// protected
TEMPLATE
code CLASS::to_tx_code(linkage<schema::code>::integer value) NOEXCEPT
{
    // validated_tx code to error code.
    switch (value)
    {
        case schema::tx_state::connected:
            return error::tx_connected;
        case schema::tx_state::preconnected:
            return error::tx_preconnected;
        case schema::tx_state::disconnected:
            return error::tx_disconnected;
        default:
            return error::unknown;
    }
}

// protected
TEMPLATE
bool CLASS::is_sufficient(const context& current,
    const context& evaluated) NOEXCEPT
{
    // Past evaluation at a lesser height and/or mtp is sufficient.
    // Increasing height/time cannot invalidate what is previously valid.
    return evaluated.flags == current.flags
        && evaluated.height <= current.height
        && evaluated.mtp <= current.mtp;
}

TEMPLATE
code CLASS::get_block_state(const header_link& link) NOEXCEPT
{
    uint64_t fees{};
    return get_block_state(fees, link);
}

TEMPLATE
code CLASS::get_block_state(uint64_t& fees, const header_link& link) NOEXCEPT
{
    // An unassociated state implies block could not have been validated.
    if (!is_associated(link))
        return error::unassociated;

    // Not found is presumed over invalid link.
    table::validated_bk::slab valid{};
    if (store_.validated_bk.get(store_.validated_bk.first(link), valid))
        return error::not_found;

    fees = valid.fees;
    return to_block_code(valid.code);
}

TEMPLATE
code CLASS::get_tx_state(const tx_link& link, const context& ctx) NOEXCEPT
{
    uint64_t fee{};
    size_t sigops{};
    return get_tx_state(fee, sigops, link, ctx);
}

TEMPLATE
code CLASS::get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
    const context& ctx) NOEXCEPT
{
    table::validated_tx::slab valid{};
    auto it = store_.validated_tx.it(link);
    if (it.self().is_terminal())
        return error::no_entry;

    // First (last pushed) with sufficient context controls state.
    do
    {
        // error::unknown return implies store inconsistency (links derived).
        if (!store_.validated_tx.get(it.self(), valid))
            return error::unknown;

        if (is_sufficient(ctx, valid.ctx))
        {
            fee = valid.fee;
            sigops = valid.sigops;
            return to_tx_code(valid.code);
        }
    }
    while (it.advance());
    return error::no_entry;
}

TEMPLATE
bool CLASS::set_block_preconfirmable(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.validated_bk.put(link, table::validated_bk::slab
    {
        {},
        schema::block_state::preconfirmable,
        0 // fees
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_block_confirmable(const header_link& link,
    uint64_t fees) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.validated_bk.put(link, table::validated_bk::slab
    {
        {},
        schema::block_state::confirmable,
        fees
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_block_unconfirmable(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.validated_bk.put(link, table::validated_bk::slab
    {
        {},
        schema::block_state::unconfirmable,
        0 // fees
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_preconnected(const tx_link& link,
    const context& ctx) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::preconnected,
        0, // fee
        0  // sigops
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_connected(const tx_link& link, const context& ctx,
    uint64_t fee, size_t sigops) NOEXCEPT
{
    using sigs = linkage<schema::sigops>;
    BC_ASSERT(sigops < system::power2<sigs::integer>(to_bits(sigs::size)));

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::connected,
        fee,
        system::possible_narrow_cast<sigs::integer>(sigops)
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_disconnected(const tx_link& link,
    const context& ctx) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return !store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::disconnected,
        0, // fee
        0  // sigops
    });
    // ========================================================================
}

// Confirmation (foreign-keyed).
// ----------------------------------------------------------------------------

// Block state.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
bool CLASS::is_associated(const header_link& link) NOEXCEPT
{
    // Guards against unnecessary hashmap search.
    // False return implies invalid link or txs are not associated.
    return !link.is_terminal() && store_.txs.exists(link);
}

TEMPLATE
bool CLASS::is_candidate_block(const header_link& link) NOEXCEPT
{
    // False return implies invalid link.
    table::header::record_height header{};
    if (!store_.header.get(link, header))
        return false;

    // False return implies no candidate at block's height.
    table::height::record candidate{};
    if (!store_.candidate.get(header.height, candidate))
        return false;

    // False return implies different candidate at block's height.
    return candidate.header_fk == link;
}

TEMPLATE
bool CLASS::is_confirmed_block(const header_link& link) NOEXCEPT
{
    // False return implies invalid link.
    table::header::record_height header{};
    if (!store_.header.get(link, header))
        return false;

    // False return implies no confirmed at block's height.
    table::height::record confirmed{};
    if (!store_.confirmed.get(header.height, confirmed))
        return false;

    // False return implies different confirmed at block's height.
    return confirmed.header_fk == link;
}

TEMPLATE
bool CLASS::is_confirmed_tx(const tx_link& link) NOEXCEPT
{
    // Not for validatation (2 additional gets).
    // False return implies invalid link or not confirmed tx.
    return is_confirmed_block(strong_by(link));
}

TEMPLATE
bool CLASS::is_confirmed_input(const input_link& link) NOEXCEPT
{
    // Not for validatation (2 additional gets).
    // False return implies invalid link or not confirmed input.
    return is_confirmed_tx(to_input_tx(link));
}

TEMPLATE
bool CLASS::is_confirmed_output(const output_link& link) NOEXCEPT
{
    // Not for validatation (2 additional gets).
    // False return implies invalid link or not confirmed output.
    return is_confirmed_tx(to_output_tx(link));
}

// Confirmation process.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
bool CLASS::strong(const header_link& link) NOEXCEPT
{
    // False return implies invalid link or unassociated.
    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, link };

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return std::all_of(txs.begin(), txs.end(), [&](const auto& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::unstrong(const header_link& link) NOEXCEPT
{
    // False return implies invalid link or unassociated.
    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, header_link::terminal };

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return std::all_of(txs.begin(), txs.end(), [&](const auto& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::is_strong_tx(const tx_link& link) NOEXCEPT
{
    // False implies invalid link or weak tx.
    // Strong identifies confirmed and pending confirmed txs.
    // Strong is reversed (reorg) by putting a terminal value.
    // Strong is only sufficient for confirmation during organizing.
    // An orgination query lock would allow this to be used vs is_confirmed_tx.
    return !strong_by(link).is_terminal();
}

TEMPLATE
bool CLASS::is_strong_prevout(const input_link& link) NOEXCEPT
{
    // False implies invalid link.
    table::input::slab_decomposed_sk in{};
    if (!store_.input.get(link, in))
        return false;

    // True implies null input, considered a strong prevout.
    // This check supersedes use of to_prevout_tx, which cannot provide
    // differentiation between null_point and missing point in the return.
    if (in.is_null())
        return true;

    // False implies prevout tx not found or not strong.
    // Assumes input is validated, avoids tx table query for output count.
    return is_strong_tx(to_tx(store_.point.get_key(in.point_fk)));
}

TEMPLATE
bool CLASS::is_strong_spent_prevout(const input_link& link) NOEXCEPT
{
    // False implies invalid link, null point or spent only by self (weak).
    const auto ins = to_spenders(link);
    if (ins.size() == one)
        return false;

    // False implies prevout not strongly spent.
    return std::none_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        // Use strong for performance benefit (confirmed would work).
        return (in != link) && is_strong_tx(to_input_tx(in));
    });
}

TEMPLATE
bool CLASS::is_confirmable_block(const header_link& link) NOEXCEPT
{
    // False implies invalid link or store inconsistency (must be inputs).
    const auto ins = to_block_inputs(link);
    if (ins.empty())
        return false;

    // False implies not confirmable block.
    return std::all_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        // Null inputs are considered strong prevouts (and not spent).
        return is_strong_prevout(in) && !is_strong_spent_prevout(in);
    });
}

// Set block state.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
bool CLASS::push(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return store_.confirmed.put(link, table::height::record{});
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return store_.candidate.put(link, table::height::record{});
    // ========================================================================
}

TEMPLATE
bool CLASS::pop() NOEXCEPT
{
    // False return implies index not initialized (no genesis).
    const auto top = get_top();
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return store_.confirmed.truncate(sub1(top));
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    // False return implies index not initialized (no genesis).
    const auto top = get_top_candidate();
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure.
    return store_.confirmed.truncate(sub1(top));
    // ========================================================================
}

// Buffer (foreign-keyed).
// ----------------------------------------------------------------------------
// Transaction buffering uses a failover cache managed by the query engine.
// (1) in-memory bimap, (2) wire streaming to disk, (3) store archive.
// Cache is maintained until validation (inputs and prevouts).
// Cache size is controlled by the block download window.
// Disk cache is created/cleared and startup/shutdown.

TEMPLATE
typename CLASS::input::cptr CLASS::get_buffered_output(
    const point&) NOEXCEPT
{
    return {};
}

TEMPLATE
typename CLASS::transaction::cptr CLASS::get_buffered_tx(
    const tx_link&) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_buffered_tx(const transaction&) NOEXCEPT
{
    return {};
}

// Neutrino (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
typename  CLASS::filter CLASS::get_filter(const header_link&) NOEXCEPT
{
    return {};
}

TEMPLATE
hash_digest CLASS::get_filter_head(const header_link&) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_filter(const header_link&, const hash_digest&,
    const filter&) NOEXCEPT
{
    return {};
}

// Bootstrap (array).
// ----------------------------------------------------------------------------

TEMPLATE
hashes CLASS::get_bootstrap(size_t height) NOEXCEPT
{
    // False return implies height exceeds confirmed top or is max_size_t.
    if (height > get_top() || height == max_size_t)
        return false;

    table::bootstrap::record boot{};
    boot.block_hashes.resize(add1(height));

    // Empty return implies empty bootstrap table or store failure.
    if (!store_.bootstrap.get(0, boot))
        return {};

    return std::move(boot.block_hashes);
}

TEMPLATE
bool CLASS::set_bootstrap(size_t height) NOEXCEPT
{
    // False return implies height exceeds confirmed top or is max_size_t.
    if (height > get_top() || height == max_size_t)
        return false;

    table::bootstrap::record boot{};
    boot.block_hashes.reserve(add1(height));

    // False return implies reorganization or store failure.
    for (auto index = zero; index <= height; ++index)
    {
        const auto header_fk = to_confirmed(index);
        if (header_fk.is_terminal())
            return false;

        boot.block_hashes.push_back(store_.header.get_key(header_fk));
    }

    // False return implies store or allocation failure.
    return store_.bootstrap.truncate(0) && store_.bootstrap.put(boot);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
