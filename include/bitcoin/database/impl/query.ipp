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

/// Setters:
/// False implies error (invalid store or parameter association).
/// Caller should assume invalid store (proper parameterization).
/// Getters:
/// Null/empty/false implies not found or error (invalid store).
/// Caller should assume not found, idependently monitor store state.

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

// Initialization.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_empty() NOEXCEPT
{
    return is_zero(store_.confirmed.count()) ||
        is_zero(store_.candidate.count());
}

TEMPLATE
size_t CLASS::get_top() NOEXCEPT
{
    BC_ASSERT(!is_empty());
    return sub1(store_.confirmed.count());
}

TEMPLATE
size_t CLASS::get_top_candidate() NOEXCEPT
{
    BC_ASSERT(!is_empty());
    return sub1(store_.candidate.count());
}

TEMPLATE
size_t CLASS::get_fork() NOEXCEPT
{
    BC_ASSERT(!is_empty());

    // Fork is greatest height with same block.
    for (auto height = get_top(); !is_zero(height); --height)
        if (to_confirmed(height) == to_candidate(height))
            return height;

    return zero;
}

TEMPLATE
size_t CLASS::get_last_associated_from(size_t height) NOEXCEPT
{
    BC_ASSERT(height != max_size_t);
    while (is_block(to_candidate(++height)));
    return --height;
}

TEMPLATE
hashes CLASS::get_all_unassociated_above(size_t height) NOEXCEPT
{
    BC_ASSERT(height != max_size_t);

    hashes out{};
    table::header::link link{};
    const auto top = get_top_candidate();

    while (height < top)
        if (!is_block((link = to_candidate(++height))))
            out.push_back(store_.header.get_key(link));

    return out;
}

TEMPLATE
hashes CLASS::get_locator(const heights& heights) NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());
    for (const auto& height: heights)
        out.push_back(store_.header.get_key(height));

    return out;
}

// Key conversion. 
// ----------------------------------------------------------------------------

TEMPLATE
header_link CLASS::to_candidate(size_t height) NOEXCEPT
{
    return store_.candidate.it(system::possible_narrow_cast<
        table::height::block::integer>(height)).self();
}

TEMPLATE
header_link CLASS::to_confirmed(size_t height) NOEXCEPT
{
    return store_.confirmed.it(system::possible_narrow_cast<
        table::height::block::integer>(height)).self();
}

TEMPLATE
header_link CLASS::to_header(const hash_digest& key) NOEXCEPT
{
    return store_.header.it(key).self();
}

TEMPLATE
point_link CLASS::to_point(const hash_digest& key) NOEXCEPT
{
    return store_.point.it(key).self();
}

TEMPLATE
txs_link CLASS::to_txs(const header_link& link) NOEXCEPT
{
    return store_.txs.it(link).self();
}

TEMPLATE
tx_link CLASS::to_tx(const hash_digest& key) NOEXCEPT
{
    return store_.tx.it(key).self();
}

TEMPLATE
output_links CLASS::to_tx_outputs(const tx_link& link) NOEXCEPT
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

TEMPLATE
input_links CLASS::to_tx_inputs(const tx_link& link) NOEXCEPT
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
tx_links CLASS::to_transactions(const header_link& link) NOEXCEPT
{
    // BUGBUG: combined search/read false/error.
    table::txs::slab out{};
    if (!store_.txs.get(link, out))
        return {};

    return std::move(out.tx_fks);
}

TEMPLATE
input_links CLASS::to_block_inputs(const header_link& link) NOEXCEPT
{
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    input_links out{};
    for (const auto& tx: txs)
    {
        const auto inputs = to_tx_inputs(tx);
        if (inputs.empty())
            return {};

        out.insert(out.end(), inputs.begin(), inputs.end());
    }

    return out;
}

// Archive (natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_header(const hash_digest& key) NOEXCEPT
{
    return store_.header.exists(key);
}

TEMPLATE
bool CLASS::is_block(const hash_digest& key) NOEXCEPT
{
    return is_block(to_header(key));
}

TEMPLATE
bool CLASS::is_tx(const hash_digest& key) NOEXCEPT
{
    return store_.tx.exists(key);
}

TEMPLATE
bool CLASS::set(const header& header, const context& ctx) NOEXCEPT
{
    return !set_link(header, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const block& block, const context& ctx) NOEXCEPT
{
    return !set_link(block, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set(const transaction& tx) NOEXCEPT
{
    return !set_link(tx).is_terminal();
}

TEMPLATE
bool CLASS::populate(const block& block) NOEXCEPT
{
    auto result = true;
    const auto ins = block.inputs_ptr();

    // TODO: evaluate concurrency for larger tx counts.
    std::for_each(ins->begin(), ins->end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const transaction& tx) NOEXCEPT
{
    auto result = true;
    const auto& ins = *tx.inputs_ptr();

    // TODO: evaluate concurrency for larger input counts.
    std::for_each(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const input& input) NOEXCEPT
{
    return ((input.prevout = get_output(input.point())));
}

// Archive (foreign-keyed).
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
    // BUGBUG: combined search/read false/error.
    table::txs::slab out{};
    if (!store_.txs.get(link, out))
        return {};

    system::hashes hashes{};
    hashes.reserve(out.tx_fks.size());

    for (const auto& fk: out.tx_fks)
        hashes.push_back(store_.tx.get_key(fk));

    return hashes;
}

TEMPLATE
CLASS::inputs_cptr CLASS::get_inputs(const tx_link& link) NOEXCEPT
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
CLASS::outputs_cptr CLASS::get_outputs(const tx_link& link) NOEXCEPT
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
CLASS::transactions_cptr CLASS::get_transactions(
    const header_link& link) NOEXCEPT
{
    // BUGBUG: combined search/read false/error.
    table::txs::slab out{};
    if (!store_.txs.get(link, out))
        return {};

    const auto txs = system::to_shared<system::chain::transaction_ptrs>();
    txs->reserve(out.tx_fks.size());

    for (const auto& fk: out.tx_fks)
        if (!push_bool(*txs, get_tx(fk)))
            return {};

    return txs;
}

TEMPLATE
CLASS::header::cptr CLASS::get_header(const header_link& link) NOEXCEPT
{
    table::header::record head{};
    if (!store_.header.get(link, head))
        return {};

    // Terminal parent implies genesis (no parent header).
    table::header::record_sk parent{};
    if ((head.parent_fk != table::header::link::terminal) &&
        !store_.header.get(head.parent_fk, parent))
        return {};

    return system::to_shared(new header
    {
        head.version,
        std::move(parent.key),
        std::move(head.merkle_root),
        head.timestamp,
        head.bits,
        head.nonce
    });
}

TEMPLATE
CLASS::block::cptr CLASS::get_block(const header_link& link) NOEXCEPT
{
    const auto header = get_header(link);
    const auto transactions = get_transactions(link);
    if (!header || !transactions)
        return {};

    return system::to_shared(new block
    {
        header,
        transactions
    });
}

TEMPLATE
CLASS::transaction::cptr CLASS::get_tx(const tx_link& link) NOEXCEPT
{
    table::transaction::only tx{};
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

    return system::to_shared(new transaction
    {
        tx.version,
        inputs,
        outputs,
        tx.locktime
    });
}

TEMPLATE
CLASS::output::cptr CLASS::get_output(const output_link& fk) NOEXCEPT
{
    table::output::only out{};
    if (!store_.output.get(fk, out))
        return {};

    return out.output;
}

TEMPLATE
CLASS::input::cptr CLASS::get_input(const input_link& fk) NOEXCEPT
{
    table::input::only_with_decomposed_sk in{};
    if (!store_.input.get(fk, in))
        return {};

    table::point::record_sk hash{};
    if (!store_.point.get(in.point_fk, hash))
        return {};

    return system::to_shared(new input
    {
        system::to_shared(new point
        {
            std::move(hash.key),
            in.point_index 
        }),
        in.script,
        in.witness,
        in.sequence
    });
}

TEMPLATE
CLASS::output::cptr CLASS::get_output(const point& prevout) NOEXCEPT
{
    if (prevout.is_null())
        return {};

    return get_output(to_tx(prevout.hash()), prevout.index());
}

TEMPLATE
CLASS::output::cptr CLASS::get_output(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    table::transaction::record_output tx{ {}, output_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record_get_one output{};
    if (!store_.puts.get(tx.output_fk, output))
        return {};

    return get_output(output.put_fk);
}

TEMPLATE
CLASS::input::cptr CLASS::get_input(const tx_link& link,
    uint32_t input_index) NOEXCEPT
{
    table::transaction::record_input tx{ {}, input_index };
    if (!store_.tx.get(link, tx))
        return {};

    table::puts::record_get_one input{};
    if (!store_.puts.get(tx.input_fk, input))
        return {};

    return get_input(input.put_fk);
}

TEMPLATE
CLASS::inputs_cptr CLASS::get_spenders(const tx_link& link,
    uint32_t output_index) NOEXCEPT
{
    const auto spenders = system::to_shared<system::chain::input_cptrs>();

    // Get search key of tx link (tx.hash).
    auto tx_hash = table::transaction::get_key(link);

    // Get point fk for tx.hash (if not found, no spenders, return empty).
    const auto hash_fk = to_point(tx_hash);
    if (hash_fk.is_terminal())
        return spenders;

    // Create shared point for attachment(s) to input.
    const auto point_ptr = system::to_shared(new point
    {
        std::move(tx_hash),
        output_index 
    });

    table::input::only_from_prevout out{ {}, point_ptr };
    const auto fp = table::input::compose(hash_fk, output_index);
    const auto it = store_.input.it(fp);

    // It is possible for fp to not be found (due to race).
    if (it.self().is_terminal())
        return spenders;

    do
    {
        if (!store_.input.get(it.self(), out))
            return {};

        spenders->push_back(out.input);
    }
    while (it.advance());
    return spenders;
}

TEMPLATE
header_link CLASS::set_link(const header& header, const context& ctx) NOEXCEPT
{
    // Parent must be missing iff its hash is null.
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
    const auto lock = store_.get_transactor();

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
    // Set is idempotent.
    const auto header_fk = set_link(block.header(), ctx);
    if (header_fk.is_terminal())
        return {};

    // Shortcircuit fk generation.
    if (store_.txs.exists(header_fk))
        return header_fk;

    tx_links links{};
    links.reserve(block.transactions_ptr->size());

    // Get/create foreign key for each tx (terminal ok).
    for (const auto& tx: *block.transactions_ptr())
        links.push_back(set_link(*tx));

    // Set is idempotent, requires that none are terminal.
    return set(header_fk, links) ? header_fk : table::header::link{};
}

TEMPLATE
tx_link CLASS::set_link(const transaction& tx) NOEXCEPT
{
    // Must have at least one input and output.
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
    const auto lock = store_.get_transactor();

    tx_fk = store_.tx.allocate(1);
    if (tx_fk.is_terminal())
        return {};

    uint32_t input_index = 0;
    linkage<schema::put> put_fk{};
    for (const auto& in: ins)
    {
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

    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return {};

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

    // Commit point and input for each input.
    table::point::link point_fk{};
    const table::point::record point{};
    auto input_fk = puts.in_fks.begin();
    for (const auto& in: ins)
    {
        const auto& prevout = in->point();

        // Continue with success if point exists.
        if (!store_.point.put_if(point_fk, prevout.hash(), point))
            return {};

        // Commit each input to its prevout fp.
        const auto fp = table::input::compose(point_fk, prevout.index());
        if (!store_.input.commit(*input_fk++, fp))
            return {};
    }

    return store_.tx.commit_link(tx_fk, key);
    // ========================================================================
}

TEMPLATE
bool CLASS::set(const header_link& link, const hashes& hashes) NOEXCEPT
{
    // Shortcircuit fk generation.
    if (is_block(link))
        return true;

    tx_links links{};
    links.reserve(hashes.size());

    // Create foreign key for each tx (terminal ok).
    for (const auto& hash: hashes)
        links.push_back(to_tx(hash));

    return set(link, links);
}

TEMPLATE
bool CLASS::set(const header_link& link, const tx_links& links) NOEXCEPT
{
    if (system::contains(links, txs_link::terminal))
        return false;

    // ========================================================================
    const auto lock = store_.get_transactor();

    // Continue with success if txs entry exists for header.
    return !store_.txs.put_if(link, table::txs::slab{ links }).is_terminal();
    // ========================================================================
}

// Validation (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_sufficient(const context& current,
    const context& evaluated) NOEXCEPT
{
    // Past evaluation at a lesser height and/or mtp is sufficient.
    // Increasing height/time cannot invalidate what is previously valid.
    return evaluated.flags  == current.flags
        && evaluated.height <= current.height
        && evaluated.mtp    <= current.mtp;
}

TEMPLATE
code CLASS::to_code(linkage<schema::code>::integer value) NOEXCEPT
{
    // validated_bk/tx code to error code.
    switch (static_cast<schema::state>(value))
    {
        case schema::state::valid:
            return error::valid;
        case schema::state::invalid:
            return error::invalid;
        case schema::state::connected:
            return error::connected;
        case schema::state::preconnected:
            return error::preconnected;
        default:
            return error::unknown;
    }
}

TEMPLATE
code CLASS::get_block_state(const header_link& link) NOEXCEPT
{
    // An unassociated state implies block could not have been validated.
    if (!store_.txs.exists(link))
        return error::unassociated;

    // A no_entry state implies associated but block not yet validated.
    const auto entry = store_.validated_bk.it(link).self();
    if (entry.is_terminal())
        return error::no_entry;

    // Handle read error independently from not found.
    table::validated_bk::slab_get_code out{};
    if (store_.validated_bk.get(entry, out))
        return error::unknown;

    return to_code(out.code);
}

TEMPLATE
code CLASS::get_tx_state(const tx_link& link, const context& ctx) NOEXCEPT
{
    table::validated_tx::slab_get_code out{};
    auto it = store_.validated_tx.it(link);
    if (it.self().is_terminal())
        return error::no_entry;

    // First (last pushed) with sufficient context controls state.
    do
    {
        if (!store_.validated_tx.get(it.self(), out))
            return error::unknown;

        if (is_sufficient(ctx, out.ctx))
            return to_code(out.code);
    }
    while (it.advance());
    return error::no_entry;
}

TEMPLATE
bool CLASS::set_block_connected(const header_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_block_valid(const header_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_block_invalid(const header_link& link,
    const code& code) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_tx_preconnected(const tx_link& link,
    const context& ctx) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_tx_connected(const tx_link& link, const context& ctx) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_tx_invalid(const tx_link& link, const context& ctx,
    const code& code) NOEXCEPT
{
    return {};
}

// Confirmation (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_block(const header_link& link) NOEXCEPT
{
    return store_.txs.exists(link);
}

TEMPLATE
bool CLASS::is_confirmed_block(const header_link& link) NOEXCEPT
{
    // TODO: relate fk/height to index.
    return {};
}

TEMPLATE
bool CLASS::is_candidate_block(const header_link& link) NOEXCEPT
{
    // TODO: relate fk/height to index.
    return {};
}

TEMPLATE
bool CLASS::is_confirmed_tx(const tx_link& link) NOEXCEPT
{
    return store_.strong_tx.exists(link);
}

TEMPLATE
bool CLASS::is_confirmed_prevout(const input_link& link) NOEXCEPT
{
    /// Enumerating tx.in_fks...
    /// For input to be valid, prevout must have existed (index not relevant).
    /// Validator can therefore check prevout.parent_fk confirmation state.
    /// Also considered confirmed if prevout.parent_fk is in current block.
    /// The prevout.parent_fk is the input.sk.tx_fk (index is not relevant).
    return {};
}


TEMPLATE
bool CLASS::is_unspent_prevout(const input_link& link) NOEXCEPT
{
    /// Enumerating tx.in_fks...
    /// Output must not be spent by confirmed transaction (height irrelevant).
    /// For each input.fp == link.fp (multimap), no strong_tx(input.parent_fk).
    /// If there is only one spender it must be self (so not confirmed).
    return {};
}

TEMPLATE
bool CLASS::is_tx_confirmable(const tx_link& link) NOEXCEPT
{
    const auto inputs = to_tx_inputs(link);
    if (inputs.empty())
        return false;

    // TODO: evaluate concurrency for larger input counts.
    return std::all_of(inputs.begin(), inputs.end(), [](const auto link)
    {
        return is_unspent_prevout(link) && is_confirmed_prevout(link);
    });
}

TEMPLATE
bool CLASS::is_block_confirmable(const header_link& link) NOEXCEPT
{
    const auto inputs = to_block_inputs(link);
    if (inputs.empty())
        return false;

    // TODO: evaluate concurrency for larger input counts.
    return std::all_of(inputs.begin(), inputs.end(), [](const auto link)
    {
        return is_unspent_prevout(link) && is_confirmed_prevout(link);
    });
}

TEMPLATE
bool CLASS::push(const header_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::pop() NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    return {};
}

// Buffer (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::transaction::cptr CLASS::get_buffered_tx(const tx_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_buffered_tx(const transaction& tx) NOEXCEPT
{
    return {};
}

// Neutrino (foreign-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::filter CLASS::get_filter(const header_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
hash_digest CLASS::get_filter_head(const header_link& link) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_filter(const header_link& link, const hash_digest& head,
    const filter& body) NOEXCEPT
{
    return {};
}

// Bootstrap (array).
// ----------------------------------------------------------------------------

TEMPLATE
hashes CLASS::get_bootstrap(size_t from, size_t to) NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::set_bootstrap(size_t height) NOEXCEPT
{
    return {};
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
