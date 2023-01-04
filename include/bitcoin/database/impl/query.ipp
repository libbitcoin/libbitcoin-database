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
// was propagated, or that a positive propagated, followed by a negative. In
// this case the original cause of the negative is presumed to be unimportant.
// Store integrity is not assured if the indexes are empty (no genesis block),
// and therefore assertions are provided for these limited situations, as
// error handling would be unnecessarily costly. The integrity presumption
// does not hold given faults in the underlying store interface. Consequently
// these are managed independently through the logging interface. Note that
// cascading failures from terminal to search key is an unnecessary perf hit.
// Note that expected stream invalidation may occur (index validation). Store
// read (paging) failures will result in an unhandled exception (termination).
// This can happen from drive disconnection (for example). Store write failures
// can result from memory remap failure, such as when the disk is full. These
// are caught and paropagated to write failures and then query failures. Given
// a reliable disk, disk full is the only commonly-expected fault condition.
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
inline bool CLASS::is_initialized() NOEXCEPT
{
    // True return implies genesis indexed (ok).
    return !is_zero(store_.confirmed.count()) &&
        !is_zero(store_.candidate.count());
}

TEMPLATE
inline size_t CLASS::get_top_confirmed() NOEXCEPT
{
    BC_ASSERT_MSG(!is_zero(store_.confirmed.count()), "empty");
    return sub1(store_.confirmed.count());
}

TEMPLATE
inline size_t CLASS::get_top_candidate() NOEXCEPT
{
    BC_ASSERT_MSG(!is_zero(store_.candidate.count()), "empty");
    return sub1(store_.candidate.count());
}

TEMPLATE
size_t CLASS::get_fork() NOEXCEPT
{
    for (auto height = get_top_confirmed(); !is_zero(height); --height)
        if (to_confirmed(height) == to_candidate(height))
            return height;

    // Should not be called during organization.
    return zero;
}

TEMPLATE
size_t CLASS::get_last_associated_from(size_t height) NOEXCEPT
{
    if (height >= height_link::terminal)
        return height_link::terminal;

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

    // A missing hash implies one or more unindexed heights (ok).
    // Due to reorganization, top may decrease intermittently.
    out.shrink_to_fit();
    return out;
}

// Key translation. 
// ----------------------------------------------------------------------------

// natural key (entry)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
inline header_link CLASS::to_candidate(size_t height) NOEXCEPT
{
    // Terminal return implies height above top candidate (ok).
    if (height >= store_.candidate.count())
        return {};

    return system::possible_narrow_cast<table::height::block::integer>(height);
}

TEMPLATE
inline header_link CLASS::to_confirmed(size_t height) NOEXCEPT
{
    // Terminal return implies height above top confirmed (ok).
    if (height >= store_.confirmed.count())
        return {};

    return system::possible_narrow_cast<table::height::block::integer>(height);
}

TEMPLATE
inline header_link CLASS::to_header(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies not archived (ok).
    return store_.header.first(key);
}

TEMPLATE
inline point_link CLASS::to_point(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies not archived (ok).
    return store_.point.first(key);
}

TEMPLATE
inline tx_link CLASS::to_tx(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies not archived (ok).
    return store_.tx.first(key);
}

TEMPLATE
inline typename CLASS::txs_link CLASS::to_txs(const header_link& link) NOEXCEPT
{
    // Terminal return implies not associated (ok).
    return store_.txs.first(link);
}

// put to tx (reverse navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
tx_link CLASS::to_input_tx(const input_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::input::get_parent in{};
    if (!store_.input.get(link, in))
        return {};

    return in.parent_fk;
}

TEMPLATE
tx_link CLASS::to_output_tx(const output_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    return out.parent_fk;
}

TEMPLATE
tx_link CLASS::to_prevout_tx(const input_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::input::slab_decomposed_fk in{};
    if (!store_.input.get(link, in))
        return {};

    // Terminal return implies null point (ok).
    if (in.is_null())
        return {};

    // Terminal return implies prevout tx not archived (ok).
    return to_tx(store_.point.get_key(in.point_fk));
}

// point to put (forward navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
input_link CLASS::to_input(const tx_link& link, uint32_t input_index) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::transaction::record_input tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    // Terminal return implies index does not exist (fault if verified).
    table::puts::record_get_one put{};
    if (!store_.puts.get(tx.puts_fk, put))
        return {};

    return put.put_fk;
}

TEMPLATE
output_link CLASS::to_output(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::transaction::record_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    // Terminal return implies index does not exist (fault if verified).
    table::puts::record_get_one put{};
    if (!store_.puts.get(tx.puts_fk, put))
        return {};

    return put.put_fk;
}

TEMPLATE
output_link CLASS::to_prevout(const input_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::input::slab_decomposed_sk in{};
    if (!store_.input.get(link, in))
        return {};

    // Terminal return implies null point (ok).
    if (in.is_null())
        return {};

    // Terminal return implies prevout tx not archived (ok).
    return to_output(to_tx(store_.point.get_key(in.point_fk)), in.point_index);
}

// tx to blocks (reverse navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
header_link CLASS::to_strong_by(const tx_link& link) NOEXCEPT
{
    // Terminal return implies not strong (ok).
    const auto fk = store_.strong_tx.first(link);
    if (fk.is_terminal())
        return {};

    // Terminal return implies serial fail (fault).
    table::strong_tx::record strong{};
    if (!store_.strong_tx.get(fk, strong))
        return {};

    // Terminal return implies strong block reverted, not strong (ok).
    return strong.header_fk;
}

// output to spenders (reverse navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
input_links CLASS::to_spenders(const point& prevout) NOEXCEPT
{
    // Empty return implies null point (ok).
    if (prevout.is_null())
        return {};

    // Empty return implies point/input not archived (ok).
    const auto point_fk = to_point(prevout.hash());
    if (point_fk.is_terminal())
        return {};

    // TODO: redundant null point check.
    // Empty return implies input not yet committed for the point (ok).
    return to_spenders(table::input::compose(point_fk, prevout.index()));
}

TEMPLATE
input_links CLASS::to_spenders(const output_link& link) NOEXCEPT
{
    // Empty return implies invalid link or serial fail (fault if verified).
    table::output::get_point out{};
    if (!store_.output.get(link, out))
        return {};

    // Empty return implies no spenders (ok).
    return to_spenders(out.parent_fk, out.index);
}

TEMPLATE
input_links CLASS::to_spenders(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    // Empty return implies no spenders (ok).
    // Null hash (get_key) implies invalid link (fault if verified).
    return to_spenders(point{ store_.tx.get_key(link), output_index });
}

// protected
TEMPLATE
input_links CLASS::to_spenders(const table::input::search_key& key) NOEXCEPT
{
    // Empty return implies null point (ok).
    if (key == table::input::null_point())
        return {};

    // Empty return implies key not found (fault if verified).
    auto it = store_.input.it(key);
    if (it.self().is_terminal())
        return {};

    input_links spenders{};
    do { spenders.push_back(it.self()); } while (it.advance());
    return spenders;
}

// block/tx to puts (forward navigation)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TEMPLATE
input_links CLASS::to_tx_inputs(const tx_link& link) NOEXCEPT
{
    // Empty return implies invalid link or serial fail (fault if verified).
    table::transaction::record_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // Empty return implies serial fail (store inconsistent).
    table::puts::record puts{};
    puts.in_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    // TODO: use pointer?
    return std::move(puts.in_fks);
}

TEMPLATE
output_links CLASS::to_tx_outputs(const tx_link& link) NOEXCEPT
{
    // Empty return implies invalid link or serial fail (fault if verified).
    table::transaction::record_puts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // Empty return implies serial fail (store inconsistent).
    table::puts::record puts{};
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.outs_fk(), puts))
        return {};

    // TODO: use pointer?
    return std::move(puts.out_fks);
}

TEMPLATE
input_links CLASS::to_block_inputs(const header_link& link) NOEXCEPT
{
    // This is 1 fk search and read for block, and 2 reads for each tx.

    // Empty return implies unassociated (ok).
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    input_links ins{};
    for (const auto& tx: txs)
    {
        // Empty return implies store inconsistency (fault).
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
    // Empty return implies unassociated (ok).
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    output_links outs{};
    for (const auto& tx: txs)
    {
        // Empty return implies store inconsistency (fault).
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
    // Empty return implies not associated (ok).
    const auto fk = to_txs(link);
    if (fk.is_terminal())
        return {};

    // Empty return implies serial fail (fault).
    table::txs::slab txs{};
    if (!store_.txs.get(fk, txs))
        return {};

    // TODO: use pointer?
    return std::move(txs.tx_fks);
}

// Archival (natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_associated(const header_link& link) NOEXCEPT
{
    // False return implies terminal or not associated (ok).
    return !link.is_terminal() && store_.txs.exists(link);
}

TEMPLATE
inline bool CLASS::is_header(const hash_digest& key) NOEXCEPT
{
    // False return implies not archived (ok).
    return store_.header.exists(key);
}

TEMPLATE
inline bool CLASS::is_block(const hash_digest& key) NOEXCEPT
{
    // False return implies not archived or not associated.
    return is_associated(to_header(key));
}

TEMPLATE
inline bool CLASS::is_tx(const hash_digest& key) NOEXCEPT
{
    // False return implies not archived (ok).
    return store_.tx.exists(key);
}

TEMPLATE
inline bool CLASS::set(const header& header, const context& ctx) NOEXCEPT
{
    // False return implies serial fail (fault).
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const block& block, const context& ctx) NOEXCEPT
{
    // False return implies serial fail (fault).
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
inline bool CLASS::set(const transaction& tx) NOEXCEPT
{
    // False return implies serial fail (fault).
    return !set_link(tx).is_terminal();
}

TEMPLATE
inline bool CLASS::populate(const input& input) NOEXCEPT
{
    // False return could be a suppressed serial fail (fault).
    // False return (and nullptr assigned) implies prevout not found (ok).
    input.prevout = get_output(input.point());
    return input.prevout != nullptr;
}

TEMPLATE
bool CLASS::populate(const transaction& tx) NOEXCEPT
{
    auto result = true;
    const auto& ins = *tx.inputs_ptr();

    // False return (and nullptrs assigned) implies not all populated (ok).
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

    // False return (and nullptrs assigned) implies not all populated (ok).
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
    // Empty return implies not associated (ok).
    const auto fk = to_txs(link);
    if (fk.is_terminal())
        return {};

    // Empty return implies serial fail (fault).
    table::txs::slab txs{};
    if (!store_.txs.get(fk, txs))
        return {};

    system::hashes hashes{};
    hashes.reserve(txs.tx_fks.size());
    for (const auto& tx_fk: txs.tx_fks)
        hashes.push_back(store_.tx.get_key(tx_fk));

    // Return of any null hash (tx.get_key) implies store inconsistency (fault).
    return hashes;
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_inputs(const tx_link& link) NOEXCEPT
{
    // nullptr return implies invalid link or serial fail (fault if verified).
    const auto fks = to_tx_inputs(link);
    if (fks.empty())
        return {};

    const auto inputs = system::to_shared<system::chain::input_cptrs>();
    inputs->reserve(fks.size());

    // nullptr return implies store inconsistency (fault).
    for (const auto& fk: fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    return inputs;
}

TEMPLATE
typename CLASS::outputs_ptr CLASS::get_outputs(const tx_link& link) NOEXCEPT
{
    // nullptr return implies invalid link or serial fail (fault if verified).
    const auto fks = to_tx_outputs(link);
    if (fks.empty())
        return {};

    const auto outputs = system::to_shared<system::chain::output_cptrs>();
    outputs->reserve(fks.size());

    // nullptr return implies store inconsistency (fault).
    for (const auto& fk: fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    return outputs;
}

TEMPLATE
typename CLASS::transactions_ptr CLASS::get_transactions(
    const header_link& link) NOEXCEPT
{
    // nullptr return implies not associated (ok).
    const auto fk = to_txs(link);
    if (fk.is_terminal())
        return {};

    // nullptr return implies serial fail (fault).
    table::txs::slab txs{};
    if (!store_.txs.get(fk, txs))
        return {};

    using namespace system;
    const auto transactions = to_shared<system::chain::transaction_ptrs>();
    transactions->reserve(txs.tx_fks.size());

    // nullptr return implies store inconsistency (fault).
    for (const auto& tx_fk: txs.tx_fks)
        if (!push_bool(*transactions, get_tx(tx_fk)))
            return {};

    return transactions;
}

TEMPLATE
typename CLASS::header::cptr CLASS::get_header(const header_link& link) NOEXCEPT
{
    // nullptr return implies invalid link or serial fail (fault if verified).
    table::header::record child{};
    if (!store_.header.get(link, child))
        return {};

    // Terminal parent implies genesis (no parent header).
    // nullptr return implies store inconsistency (missing required parent).
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
    // nullptr return implies invalid link or serial fail (fault if verified).
    const auto header = get_header(link);
    if (!header)
        return {};

    // nullptr return implies not associated (ok).
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
    // nullptr return implies invalid link or serial fail (fault if verified).
    table::transaction::only tx{};
    if (!store_.tx.get(link, tx))
        return {};

    // nullptr return implies store inconsistency (fault).
    table::puts::record puts{};
    puts.in_fks.resize(tx.ins_count);
    puts.out_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    const auto inputs = system::to_shared<system::chain::input_cptrs>();
    const auto outputs = system::to_shared<system::chain::output_cptrs>();
    inputs->reserve(tx.ins_count);
    outputs->reserve(tx.outs_count);

    // nullptr return implies store inconsistency (fault).
    for (const auto& fk: puts.in_fks)
        if (!push_bool(*inputs, get_input(fk)))
            return {};

    // nullptr return implies store inconsistency (fault).
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
    // nullptr return implies invalid link or serial fail (fault if verified).
    table::output::only out{};
    if (!store_.output.get(link, out))
        return {};

    return out.output;
}

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const input_link& link) NOEXCEPT
{
    // nullptr return implies invalid link or serial fail (fault if verified).
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
    // nullptr return implies invalid link or serial fail (fault if verified).
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
typename CLASS::inputs_ptr CLASS::get_spenders(const output_link& link) NOEXCEPT
{
    // nullptr return implies invalid link or serial fail (fault if verified).
    table::output::slab out{};
    if (!store_.output.get(link, out))
        return {};

    // Empty return implies no spenders.
    return get_spenders(out.parent_fk, out.index);
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const point& prevout) NOEXCEPT
{
    // nullptr return implies null point (ok).
    // Shortcircuits get_output(to_tx(null_hash)) fault.
    if (prevout.is_null())
        return {};

    // nullptr return implies tx and/or index not found (ok).
    return get_output(to_tx(prevout.hash()), prevout.index());
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    // nullptr return implies invalid link or tx and/or index not found.
    return get_output(to_output(link, output_index));
}

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const tx_link& link,
    uint32_t input_index) NOEXCEPT
{
    // nullptr return implies invalid link or tx and/or index not found.
    return get_input(to_input(link, input_index));
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    const auto fks = to_spenders(link, output_index);
    const auto spenders = system::to_shared<system::chain::input_cptrs>();
    spenders->reserve(fks.size());

    // nullptr return implies store inconsistency (fault).
    for (const auto& fk: fks)
        if (!push_bool(*spenders, get_input(fk)))
            return {};

    // Empty return implies fault or no spenders (indistinguishable).
    return spenders;
}

// protected
TEMPLATE
inline typename CLASS::input_key CLASS::make_foreign_point(
    const point& prevout) NOEXCEPT
{
    // Terminal fp return [terminal, null_index] implies null point (ok).
    if (prevout.is_null())
        return table::input::compose(tx_link::terminal, prevout.index());

    // Reuse point hash fk if archived.
    auto point_fk = to_point(prevout.hash());

    // Create point hash fk if not found.
    if (point_fk.is_terminal())
    {
        // Terminal fp return implies allocation failure (fault).
        const table::point::record empty{};
        if (!store_.point.put_link(point_fk, prevout.hash(), empty))
            return {};
    }

    // Terminal fp return implies fault or null point (indistinguishable).
    return table::input::compose(point_fk, prevout.index());
}

TEMPLATE
tx_link CLASS::set_link(const transaction& tx) NOEXCEPT
{
    // Terminal return implies no inputs and/or outputs (ok).
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

    // Terminal return implies allocation failure (fault).
    tx_fk = store_.tx.allocate(1);
    if (tx_fk.is_terminal())
        return {};

    uint32_t input_index = 0;
    linkage<schema::put> put_fk{};
    for (const auto& in: ins)
    {
        // Terminal return implies allocation/serial fail (fault).
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
        // Terminal return implies allocation/serial fail (fault).
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

    // Terminal return implies allocation/serial fail (fault).
    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return {};

    // Terminal return implies allocation/serial fail (fault).
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

    // Terminal return implies memory access fail (fault).
    // Commit each input to its seach key (prevout foreign point).
    auto input_fk = puts.in_fks.begin();
    for (const auto& in: ins)
        if (!store_.input.commit(*input_fk++, make_foreign_point(in->point())))
            return {};

    // Terminal return implies memory access failure (fault).
    return store_.tx.commit_link(tx_fk, key);
    // ========================================================================
}

TEMPLATE
header_link CLASS::set_link(const block& block, const context& ctx) NOEXCEPT
{
    // Terminal return implies allocation/serial fail (fault).
    const auto header_fk = set_link(block.header(), ctx);
    if (header_fk.is_terminal())
        return {};

    // Non-terminal return implies shortcircuit for associated (ok).
    if (is_associated(header_fk))
        return header_fk;

    tx_links links{};
    links.reserve(block.transactions_ptr()->size());

    // Get/create foreign key for each tx (terminal possible).
    for (const auto& tx: *block.transactions_ptr())
        links.push_back(set_link(*tx));

    // Set is idempotent, requires that none are terminal.
    // Terminal return implies allocation failure (fault).
    return set(header_fk, links) ? header_fk : table::header::link{};
}

TEMPLATE
header_link CLASS::set_link(const header& header, const context& ctx) NOEXCEPT
{
    // Parent must be missing iff its hash is null.
    // Terminal return implies inconsistent parent reference (ok).
    const auto& parent_sk = header.previous_block_hash();
    const auto parent_fk = to_header(parent_sk);
    if (parent_fk.is_terminal() != (parent_sk == system::null_hash))
        return {};

    // Non-terminal return implies header exists (ok).
    const auto key = header.hash();
    auto header_fk = to_header(key);
    if (!header_fk.is_terminal())
        return header_fk;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Terminal return implies allocation failure (fault).
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
    // True return implies shortcircuit create for terminal/unassociated (ok).
    if (is_associated(link))
        return true;

    tx_links links{};
    links.reserve(hashes.size());

    // Create foreign key for each tx (terminal possible).
    for (const auto& hash: hashes)
        links.push_back(to_tx(hash));

    // False return implies hash not found (ok) or allocation fail (fault).
    return set(link, links);
}

TEMPLATE
bool CLASS::set(const header_link& link, const tx_links& links) NOEXCEPT
{
    // True return implies terminal or unassociated (ok).
    if (is_associated(link))
        return true;

    // False return implies terminal limits in collection (prior failures).
    if (system::contains(links, txs_link::terminal))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure (fault).
    return store_.txs.put(link, table::txs::slab{ {}, links });
    // ========================================================================
}

// Validation (foreign-keyed).
// ----------------------------------------------------------------------------

// protected
TEMPLATE
inline code CLASS::to_block_code(linkage<schema::code>::integer value) NOEXCEPT
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
inline code CLASS::to_tx_code(linkage<schema::code>::integer value) NOEXCEPT
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
inline bool CLASS::is_sufficient(const context& current,
    const context& evaluated) NOEXCEPT
{
    // Past evaluation at a lesser height and/or mtp is sufficient.
    // Increasing height/time cannot invalidate what is previously valid.
    return evaluated.flags == current.flags
        && evaluated.height <= current.height
        && evaluated.mtp <= current.mtp;
}

TEMPLATE
height_link CLASS::get_header_height(const header_link& link) NOEXCEPT
{
    // Terminal return implies invalid link or serial fail (fault if verified).
    table::header::record_height header{};
    if (!store_.header.get(link, header))
        return {};

    return header.height;
}

TEMPLATE
code CLASS::get_block_state(const header_link& link) NOEXCEPT
{
    // unassociated return implies block could not have been validated (ok).
    if (!is_associated(link))
        return error::unassociated;

    // unvalidated return implies no validation record (ok).
    const auto fk = store_.validated_bk.first(link);
    if (fk.is_terminal())
        return error::unvalidated;

    // integrity return implies serial fail (fault).
    table::validated_bk::slab_get_code valid{};
    if (!store_.validated_bk.get(fk, valid))
        return error::integrity;

    // Other code implies validation record found (ok).
    return to_block_code(valid.code);
}

TEMPLATE
code CLASS::get_block_state(uint64_t& fees, const header_link& link) NOEXCEPT
{
    // unassociated return implies block could not have been validated (ok).
    if (!is_associated(link))
        return error::unassociated;

    // unvalidated return implies no validation record (ok).
    const auto fk = store_.validated_bk.first(link);
    if (fk.is_terminal())
        return error::unvalidated;

    // integrity return implies serial fail (fault).
    table::validated_bk::slab valid{};
    if (!store_.validated_bk.get(fk, valid))
        return error::integrity;

    fees = valid.fees;

    // Other code implies validation record found (ok).
    return to_block_code(valid.code);
}

TEMPLATE
code CLASS::get_tx_state(const tx_link& link, const context& ctx) NOEXCEPT
{
    // unvalidated return implies no validation records (ok).
    auto it = store_.validated_tx.it(link);
    if (it.self().is_terminal())
        return error::unvalidated;

    // First (last pushed) with sufficient context controls state.
    table::validated_tx::slab_get_code valid{};
    do
    {
        // integrity return implies serial fail (fault).
        if (!store_.validated_tx.get(it.self(), valid))
            return error::integrity;

        // Other code implies validation record found (ok).
        if (is_sufficient(ctx, valid.ctx))
            return to_tx_code(valid.code);
    }
    while (it.advance());

    // unvalidated return implies no validation record of matching context (ok).
    return error::unvalidated;
}

TEMPLATE
code CLASS::get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
    const context& ctx) NOEXCEPT
{
    // unvalidated return implies no validation records (ok).
    auto it = store_.validated_tx.it(link);
    if (it.self().is_terminal())
        return error::unvalidated;

    // First (last pushed) with sufficient context controls state.
    table::validated_tx::slab valid{};
    do
    {
        // integrity return implies serial fail (fault).
        if (!store_.validated_tx.get(it.self(), valid))
            return error::integrity;

        // Other code implies validation record found (ok).
        if (is_sufficient(ctx, valid.ctx))
        {
            fee = valid.fee;
            sigops = valid.sigops;
            return to_tx_code(valid.code);
        }
    }
    while (it.advance());

    // unvalidated return implies no validation record of matching context (ok).
    return error::unvalidated;
}

TEMPLATE
bool CLASS::set_block_preconfirmable(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation fail (fault).
    return store_.validated_bk.put(link, table::validated_bk::slab
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

    // False return implies allocation fail (fault).
    return store_.validated_bk.put(link, table::validated_bk::slab
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

    // False return implies allocation fail (fault).
    return store_.validated_bk.put(link, table::validated_bk::slab
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

    // False return implies allocation fail (fault).
    return store_.validated_tx.put(link, table::validated_tx::slab
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

    // False return implies allocation fail (fault).
    return store_.validated_tx.put(link, table::validated_tx::slab
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

    // False return implies allocation fail (fault).
    return store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::disconnected,
        0, // fee
        0  // sigops
    });
    // ========================================================================
}

// Block status (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_candidate_block(const header_link& link) NOEXCEPT
{
    // False return implies invalid link or serial fail (fault if verified).
    const auto height = get_header_height(link);
    if (height.is_terminal())
        return false;

    // False return implies no candidate at height (ok).
    table::height::record candidate{};
    if (!store_.candidate.get(height, candidate))
        return false;

    // False return implies different candidate at block height (ok).
    return candidate.header_fk == link;
}

TEMPLATE
inline bool CLASS::is_confirmed_block(const header_link& link) NOEXCEPT
{
    // False return implies invalid link or serial fail (fault if verified).
    const auto height = get_header_height(link);
    if (height.is_terminal())
        return false;

    // False return implies no confirmed at height (ok).
    table::height::record confirmed{};
    if (!store_.confirmed.get(height, confirmed))
        return false;

    // False return implies different confirmed at block height (ok).
    return confirmed.header_fk == link;
}

TEMPLATE
inline bool CLASS::is_confirmed_tx(const tx_link& link) NOEXCEPT
{
    // Not for validatation (2 additional gets).
    // Terminal return implies invalid/serial fail (fault) or not strong (ok).
    const auto fk = to_strong_by(link);
    if (fk.is_terminal())
        return false;

    // False return implies serial fail (fault) or not confirmed (ok).
    return is_confirmed_block(fk);
}

TEMPLATE
inline bool CLASS::is_confirmed_input(const input_link& link) NOEXCEPT
{
    // Not for validatation (2 additional gets).
    // False return implies invalid link or serial fail (fault if verified).
    const auto fk = to_input_tx(link);
    if (fk.is_terminal())
        return false;

    // False return implies serial fail (fault) or not confirmed (ok).
    return is_confirmed_tx(fk);
}

TEMPLATE
inline bool CLASS::is_confirmed_output(const output_link& link) NOEXCEPT
{
    // Not for validatation (2 additional gets).
    // False return implies invalid link or serial fail (fault if verified).
    const auto fk = to_output_tx(link);
    if (fk.is_terminal())
        return false;

    // False return implies serial fail (fault) or not confirmed (ok).
    return is_confirmed_tx(fk);
}

// Confirmation.
// ----------------------------------------------------------------------------
// Strong identifies confirmed and pending confirmed txs.
// Strong is only sufficient for confirmation during organizing.

TEMPLATE
bool CLASS::is_spent(const input_link& link) NOEXCEPT
{
    // False return implies invalid link or serial fail (fault if verified).
    table::input::slab_composite_sk input{};
    if (!store_.input.get(link, input))
        return false;

    // False implies invalid, null point (0), self only (1) (ambiguous),
    // serial fail (fault) or prevout not strongly spent (ok).
    return is_spent_point(link, input.key);
}

// protected
TEMPLATE
bool CLASS::is_spent_point(const input_link& self,
    const table::input::search_key& key) NOEXCEPT
{
    // False implies invalid, null point (0), or self only (1) (ambiguous).
    const auto ins = to_spenders(key);
    if (ins.size() <= one)
        return false;

    // False implies serial fail (fault) or prevout not strongly spent (ok).
    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        // Use strong for performance benefit (confirmed would work).
        return in != self && !to_strong_by(to_input_tx(in)).is_terminal();
    });
}

TEMPLATE
bool CLASS::is_mature(const input_link& link, size_t height) NOEXCEPT
{
    // False return implies invalid link or serial fail (fault).
    table::input::slab_decomposed_fk input{};
    if (!store_.input.get(link, input))
        return false;

    // True implies strong (prevout of null input is always mature) (ok).
    if (input.is_null())
        return true;

    // False return implies serial fail (fault) or not strong (ok).
    return is_mature_point(input.point_fk, height);
}

// protected
TEMPLATE
bool CLASS::is_mature_point(const point_link& link, size_t height) NOEXCEPT
{
    // False return implies serial fail (fault) or not strong (ok).
    const auto tx_fk = to_tx(store_.point.get_key(link));
    const auto header_fk = to_strong_by(tx_fk);
    if (header_fk.is_terminal())
        return false;

    // False return implies store integrity failure (fault).
    table::transaction::record_get_coinbase tx{};
    if (!store_.tx.get(tx_fk, tx))
        return false;

    // True return implies strong non-coinbase (ok).
    if (!tx.coinbase)
        return true;

    // Terminal return implies invalid link, serial fail or race (fault).
    const auto prevout_height = get_header_height(header_fk);
    if (prevout_height.is_terminal())
        return false;

    return transaction::is_coinbase_mature(prevout_height, height);
}

TEMPLATE
bool CLASS::is_confirmable_block(const header_link& link,
    size_t height) NOEXCEPT
{
    // False implies invalid link or store inconsistency (fault if verified).
    const auto ins = to_block_inputs(link);
    if (ins.empty())
        return false;

    // False implies not confirmable block (ok).
    return std::all_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        table::input::slab_composite_sk input{};
        return store_.input.get(in, input)
            && is_mature(input.point_fk(), height) && !is_spent(in, input.key);
    });
}

TEMPLATE
bool CLASS::set_strong(const header_link& link) NOEXCEPT
{
    // False return implies serial fail or unassociated (ambiguous).
    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, link };

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation fail (fault).
    return std::all_of(txs.begin(), txs.end(), [&](const tx_link& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_unstrong(const header_link& link) NOEXCEPT
{
    // False return implies serial fail or unassociated (ambiguous).
    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, header_link::terminal };

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation fail (fault).
    return std::all_of(txs.begin(), txs.end(), [&](const tx_link& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::initialize(const block& genesis) NOEXCEPT
{
    BC_ASSERT(!is_initialized());
    BC_ASSERT(genesis.transactions_ptr()->size() == one);

    // ========================================================================
    const auto scope = store_.get_transactor();

    const context ctx{};
    if (!set(genesis, ctx))
        return false;

    // Genesis block can have only null inputs.
    constexpr auto fees = 0u;
    constexpr auto sigops = 0u;
    const auto link = to_header(genesis.hash());

    return set_strong(header_link{ 0 })
        && set_tx_connected(tx_link{ 0 }, ctx, fees, sigops)
        && set_block_confirmable(link, fees)
        && push_candidate(link)
        && push_confirmed(link);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure (fault).
    const table::height::record confirmed{ {}, link };
    return store_.confirmed.put(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure (fault).
    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    // False return implies index not initialized (no genesis).
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure (fault).
    return store_.confirmed.truncate(top);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    // False return implies index not initialized (no genesis).
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // False return implies allocation failure (fault).
    return store_.candidate.truncate(top);
    // ========================================================================
}

// Address (natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
hash_digest CLASS::address_hash(const output& output) NOEXCEPT
{
    hash_digest digest{};
    system::hash::sha256::copy sink(digest);
    output.to_data(sink);
    sink.flush();
    return digest;
}

TEMPLATE
output_link CLASS::get_address(const hash_digest& key) NOEXCEPT
{
    // Terminal return implies address not found (ok).
    const auto fk = store_.address.first(key);
    if (fk.is_terminal())
        return {};

    // Terminal return implies serial fail (fault).
    table::address::record address{};
    if (!store_.address.get(fk, address))
        return {};

    return address.output_fk;
}

TEMPLATE
bool CLASS::set_address(const hash_digest& key,
    const output_link& link) NOEXCEPT
{
    // False return implies allocation fail (fault).
    return store_.address.put(key, table::address::record
    {
        {},
        link
    });
}

TEMPLATE
bool CLASS::set_address(const output& output) NOEXCEPT
{
    // False return implies allocation fail (fault).
    return set_address(address_hash(output), output);
}

// Neutrino (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::filter CLASS::get_filter(const header_link& link) NOEXCEPT
{
    // nullptr return implies neutrino entry not found (ok).
    const auto fk = store_.neutrino.first(link);
    if (fk.is_terminal())
        return {};

    // nullptr return implies serial fail (fault).
    table::neutrino::slab_get_filter neutrino{};
    if (!store_.neutrino.get(fk, neutrino))
        return {};

    // TODO: use pointer?
    return std::move(neutrino.filter);
}

TEMPLATE
hash_digest CLASS::get_filter_head(const header_link& link) NOEXCEPT
{
    // nullptr return implies neutrino entry not found (ok).
    const auto fk = store_.neutrino.first(link);
    if (fk.is_terminal())
        return {};

    // nullptr return implies serial fail (fault).
    table::neutrino::slab_get_head neutrino{};
    if (!store_.neutrino.get(fk, neutrino))
        return {};

    return std::move(neutrino.filter_head);
}

TEMPLATE
bool CLASS::set_filter(const header_link& link, const hash_digest& filter_head,
    const filter& filter) NOEXCEPT
{
    // False return implies allocation fail (fault).
    return store_.neutrino.put(link, table::neutrino::slab_put_ref
    {
        {},
        filter_head,
        filter
    });
}

// Buffer (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::transaction::cptr CLASS::get_buffered_tx(
    const tx_link& link) NOEXCEPT
{
    // nullptr return implies buffered tx not found (ok).
    const auto fk = store_.buffer.first(link);
    if (fk.is_terminal())
        return {};

    // nullptr return implies serial fail (fault).
    table::buffer::slab_ptr buffer{};
    if (!store_.buffer.get(fk, buffer))
        return {};

    return buffer.tx;
}

TEMPLATE
bool CLASS::set_buffered_tx(const tx_link& link,
    const transaction& tx) NOEXCEPT
{
    // False return implies allocation fail (fault).
    return store_.buffer.put(link, table::buffer::slab_put_ref
    {
        {},
        tx
    });
}

// Bootstrap (array).
// ----------------------------------------------------------------------------

TEMPLATE
hashes CLASS::get_bootstrap() NOEXCEPT
{
    table::bootstrap::record boot{};
    boot.block_hashes.resize(store_.bootstrap.count());

    // Empty return implies empty table (ok) or store failure (fault).
    if (!store_.bootstrap.get(0, boot))
        return {};

    // TODO: use pointer?
    return std::move(boot.block_hashes);
}

TEMPLATE
bool CLASS::set_bootstrap(size_t height) NOEXCEPT
{
    // False return implies height exceeds confirmed top (ok).
    if (height > get_top_confirmed())
        return false;

    table::bootstrap::record boot{};
    boot.block_hashes.reserve(add1(height));

    // False return implies reorganization race (ok).
    for (auto index = zero; index <= height; ++index)
    {
        const auto header_fk = to_confirmed(index);
        if (header_fk.is_terminal())
            return false;

        boot.block_hashes.push_back(store_.header.get_key(header_fk));
    }

    // False return implies truncate or allocation failure (fault).
    return store_.bootstrap.truncate(0) && store_.bootstrap.put(boot);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
