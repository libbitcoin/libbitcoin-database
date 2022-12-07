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
#ifndef LIBBITCOIN_DATABASE_QUERIES_INTERNAL_IPP
#define LIBBITCOIN_DATABASE_QUERIES_INTERNAL_IPP

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_USE_OF_MOVED_OBJECT)

// fk setters(chain_ref)
// ----------------------------------------------------------------------------

TEMPLATE
table::transaction::link CLASS::set_tx_link(
    const system::chain::transaction& tx) NOEXCEPT
{
    // Must have at least one input and output.
    if (tx.is_empty())
        return {};

    // Return with success if tx exists.
    const auto key = tx.hash(false);
    auto tx_fk = store_.tx.it(key).self();
    if (!tx_fk.is_terminal())
        return tx_fk;

    // Declare puts record.
    const auto& ins = *tx.inputs_ptr();
    const auto& outs = *tx.outputs_ptr();
    table::puts::record puts{};
    puts.put_fks.reserve(outs.size() + ins.size());

    // BEGIN TRANSACTION
    // ------------------------------------------------------------------------
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

        puts.put_fks.push_back(put_fk);
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

        puts.put_fks.push_back(put_fk);
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
    auto input_fk = puts.put_fks.begin();
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
    // ------------------------------------------------------------------------
    // END TRANSACTION
}

TEMPLATE
table::header::link CLASS::set_header_link(const system::chain::header& header,
    const context& ctx) NOEXCEPT
{
    // Parent must be missing iff its hash is null.
    const auto& parent_sk = header.previous_block_hash();
    const auto parent_fk = store_.header.it(parent_sk).self();
    if (parent_fk.is_terminal() != (parent_sk == system::null_hash))
        return {};

    // Return with success if header exists.
    const auto key = header.hash();
    auto header_fk = store_.header.it(key).self();
    if (!header_fk.is_terminal())
        return header_fk;

    // BEGIN TRANSACTION
    // ------------------------------------------------------------------------
    const auto lock = store_.get_transactor();

    return store_.header.put_link(key, table::header::record_put_ref
    {
        {},
        ctx,
        parent_fk,
        header
    });
    // ------------------------------------------------------------------------
    // END TRANSACTION
}

TEMPLATE
table::header::link CLASS::set_block_link(const system::chain::block& block,
    const context& ctx) NOEXCEPT
{
    // Set is idempotent.
    const auto header_fk = set_header_link(block.header(), ctx);
    if (header_fk.is_terminal())
        return {};

    // Shortcircuit (redundant with set_txs put_if).
    if (store_.txs.exists(header_fk))
        return true;

    // Get/create foreign key for each tx (set is idempotent).
    table::txs::slab set{};
    const auto& txs = *block.transactions_ptr();
    set.tx_fks.reserve(txs.size());
    for (const auto& tx: txs)
        set.tx_fks.push_back(set_tx_link(*tx));

    // Set is idempotent, requires that none are terminal.
    return set_txs(header_fk, set) ? header_fk : table::header::link{};
}

TEMPLATE
bool CLASS::set_txs(const table::header::link& fk,
    const table::txs::slab& set) NOEXCEPT
{
    // Fail if any tx_fk is terminal.
    if (system::contains(set.tx_fks, table::txs::link::terminal))
        return false;

    // BEGIN TRANSACTION
    // ------------------------------------------------------------------------
    const auto lock = store_.get_transactor();

    // Continue with success if txs entry exists for header.
    return !store_.txs.put_if(fk, set).is_terminal();
    // ------------------------------------------------------------------------
    // END TRANSACTION
}

// chain_ptr getters(fk)
// ============================================================================

TEMPLATE
CLASS::header::cptr CLASS::get_header(const table::header::link& fk) NOEXCEPT
{
    using namespace system::chain;
    table::header::record head{};
    if (!store_.header.get(fk, head))
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
CLASS::block::cptr CLASS::get_block(const table::header::link& fk) NOEXCEPT
{
    using namespace system::chain;
    const auto head = get_header(fk);
    if (!head)
        return {};

    table::txs::slab set{};
    if (!store_.txs.get(fk, set))
        return {};

    const auto txs = system::to_shared<transaction_ptrs>();
    txs->reserve(set.tx_fks.size());
    for (const auto& tx_fk: set.tx_fks)
    {
        const auto tx = get_tx(tx_fk);
        if (!tx) return {};
        txs->push_back(tx);
    }

    return system::to_shared(new block
    {
        head,
        txs
    });
}

TEMPLATE
system::hashes CLASS::get_txs(const table::header::link& fk) NOEXCEPT
{
    table::txs::slab set{};
    if (!store_.txs.get(fk, set))
        return {};

    system::hashes hashes{};
    hashes.reserve(set.tx_fks.size());
    table::transaction::record_sk tx{};
    for (const auto& tx_fk: set.tx_fks)
    {
        if (!store_.tx.get(tx_fk, tx)) return {};
        hashes.push_back(std::move(tx.key));            
    }

    return hashes;
}

TEMPLATE
CLASS::transaction::cptr CLASS::get_tx(
    const table::transaction::link& fk) NOEXCEPT
{
    using namespace system::chain;
    table::transaction::only tx{};
    if (!store_.tx.get(fk, tx))
        return {};

    table::puts::record puts{};
    puts.put_fks.resize(tx.ins_count + tx.outs_count);
    if (!store_.puts.get(tx.ins_fk, puts))
        return {};

    auto it = puts.put_fks.begin();
    const auto inputs_end = std::next(it, tx.ins_count);
    const auto ins = system::to_shared<input_cptrs>();
    ins->reserve(tx.ins_count);
    while (it != inputs_end)
    {
        const auto in = get_input(*it++);
        if (!in) return {};
        ins->push_back(in);
    }

    const auto outputs_end = puts.put_fks.end();
    const auto outs = system::to_shared<output_cptrs>();
    outs->reserve(tx.outs_count);
    while (it != outputs_end)
    {
        const auto out = get_output(*it++);
        if (!out) return {};
        outs->push_back(out);
    }

    return system::to_shared(new transaction
    {
        tx.version,
        ins,
        outs,
        tx.locktime
    });
}

TEMPLATE
CLASS::input::cptr CLASS::get_input(const table::input::link& fk) NOEXCEPT
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
CLASS::output::cptr CLASS::get_output(const table::output::link& fk) NOEXCEPT
{
    table::output::only out{};
    if (!store_.output.get(fk, out))
        return {};

    return out.output;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
