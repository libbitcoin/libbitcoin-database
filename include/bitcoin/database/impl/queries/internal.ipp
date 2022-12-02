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
table::header::link CLASS::set_header_(const system::chain::header& header,
    const context& context) NOEXCEPT
{
    // Parent must be missing iff its hash is null.
    // Iterator must be released before subsequent header put.
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
        context,
        parent_fk,
        header
    });
    // ------------------------------------------------------------------------
    // END TRANSACTION
}

TEMPLATE
table::transaction::link CLASS::set_tx_(
    const system::chain::transaction& tx) NOEXCEPT
{
    // Must have at least one input and output.
    if (tx.is_empty())
        return {};

    // Return with success if tx exists.
    const auto key = tx.hash(false);
    auto tx_pk = store_.tx.it(key).self();
    if (!tx_pk.is_terminal())
        return tx_pk;

    // Declare puts record.
    const auto& ins = *tx.inputs_ptr();
    const auto& outs = *tx.outputs_ptr();
    const auto count = outs.size() + ins.size();
    table::puts::record puts{};
    puts.put_fks.reserve(count);

    // BEGIN TRANSACTION
    // ------------------------------------------------------------------------
    const auto lock = store_.get_transactor();

    // Allocate one transaction.
    tx_pk = store_.tx.allocate(1);
    if (tx_pk.is_terminal())
        return {};

    // Allocate and Set inputs, queue each put.
    uint32_t input_index = 0;
    linkage<schema::put> put_pk{};
    for (const auto& in: ins)
    {
        if (!store_.input.set_link(put_pk, table::input::slab_put_ref
        {
            {},
            tx_pk,
            input_index++,
            *in
        }))
        {
            return {};
        }

        puts.put_fks.push_back(put_pk);
    }

    // Commit outputs, queue each put.
    uint32_t output_index = 0;
    for (const auto& out: outs)
    {
        if (!store_.output.put_link(put_pk, table::output::slab_put_ref
        {
            {},
            tx_pk,
            output_index++,
            *out
        }))
        {
            return {};
        }

        puts.put_fks.push_back(put_pk);
    }

    // Commit puts (defined above).
    const auto puts_pk = store_.puts.put_link(puts);
    if (puts_pk.is_terminal())
        return {};

    // Set transaction.
    // TODO: optimize with ptr/ref writer.
    if (!store_.tx.set(tx_pk, table::transaction::record
    {
        {},
        tx.is_coinbase(),
        system::possible_narrow_cast<uint32_t>(tx.serialized_size(false)),
        system::possible_narrow_cast<uint32_t>(tx.serialized_size(true)),
        tx.locktime(),
        tx.version(),
        system::possible_narrow_cast<uint32_t>(ins.size()),
        system::possible_narrow_cast<uint32_t>(outs.size()),
        puts_pk
    }))
    {
        return {};
    }

    // Commit point and input for each input.
    table::point::link point_pk{};
    const table::point::record point{};
    auto input_fk = puts.put_fks.begin();
    for (const auto& in: ins)
    {
        const auto& prevout = in->point();

        // Continue with success if point exists.
        if (!store_.point.put_if(point_pk, prevout.hash(), point))
            return {};

        // Commit each input_fk to its prevout fp.
        if (!store_.input.commit(*input_fk++, table::input::to_point(point_pk,
            prevout.index())))
            return {};
    }

    // Commit transaction to its hash and return link (tx_pk or terminal).
    return store_.tx.commit_link(tx_pk, key);
    // ------------------------------------------------------------------------
    // END TRANSACTION
}

TEMPLATE
table::header::link CLASS::set_block_(const system::chain::block& block,
    const context& context) NOEXCEPT
{
    // Set is idempotent.
    const auto header_fk = set_header_(block.header(), context);
    if (header_fk.is_terminal())
        return {};

    // Shortcircuit (redundant with set_txs_ put_if).
    if (store_.txs.exists(header_fk))
        return true;

    // Get/create foreign key for each tx (set is idempotent).
    table::txs::slab keys{};
    const auto& txs = *block.transactions_ptr();
    keys.tx_fks.reserve(txs.size());
    for (const auto& tx: txs)
        keys.tx_fks.push_back(set_tx_(*tx));

    // Set is idempotent, requires that none are terminal.
    return set_txs_(header_fk, keys) ? header_fk : table::header::link{};
}

TEMPLATE
bool CLASS::set_txs_(const table::header::link& key,
    const table::txs::slab& txs) NOEXCEPT
{
    // Continue with success if txs exists for header.
    if (system::contains(txs.tx_fks, table::txs::link::terminal))
        return false;


    // BEGIN TRANSACTION
    // ------------------------------------------------------------------------
    const auto lock = store_.get_transactor();

    return !store_.txs.put_if(key, txs).is_terminal();
    // ------------------------------------------------------------------------
    // END TRANSACTION
}

// chain_ptr getters(fk)
// ----------------------------------------------------------------------------

TEMPLATE
system::chain::transaction::cptr CLASS::get_tx(
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

    // Initialize input/output fk iterator.
    auto it = puts.put_fks.begin();
    const auto inputs_end = std::next(it, tx.ins_count);
    const auto outputs_end = puts.put_fks.end();

    // Get inputs.
    const auto ins = system::to_shared<input_cptrs>();
    ins->reserve(tx.ins_count);
    table::point::record_sk hash{};
    table::input::only_with_decomposed_sk in{};
    for (; it != inputs_end; ++it)
    {
        if (!store_.input.get(*it, in) ||
            !store_.point.get(in.point_fk, hash))
            return {};

        ins->emplace_back(new input
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

    // Get outputs.
    const auto outs = system::to_shared<output_cptrs>();
    outs->reserve(tx.outs_count);
    table::output::only out{};
    for (; it != outputs_end; ++it)
    {
        if (!store_.output.get(*it, out))
            return {};

        outs->push_back(out.output);
    }

    // tx ctor casts inputs_ptr/outputs_ptr to inputs_cptr/outputs_cptr.
    return system::to_shared(new transaction
    {
        tx.version,
        ins,
        outs,
        tx.locktime
    });
}

TEMPLATE
system::chain::header::cptr CLASS::get_header(
    const table::header::link& fk) NOEXCEPT
{
    using namespace system::chain;
    table::header::record head{};
    if (!store_.header.get(fk, head))
        return {};

    // terminal (default) parent implies genesis, otherwise it must resolve.
    table::header::record_sk parent{};
    if ((head.parent_fk != table::header::link::terminal) &&
        !store_.header.get(head.parent_fk, parent))
        return {};

    // cannot be retrieved as an instance due to parent key.
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
system::chain::block::cptr CLASS::get_block(
    const table::header::link& fk) NOEXCEPT
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
        if (!tx)
            return {};

        txs->push_back(tx);
    }

    // block ctor casts transactions_ptr to transactions_cptr.
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
        if (!store_.tx.get(tx_fk, tx))
            return {};

        hashes.push_back(std::move(tx.key));            
    }

    return hashes;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
