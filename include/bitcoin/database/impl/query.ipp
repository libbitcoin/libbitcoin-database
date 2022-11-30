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

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

TEMPLATE
CLASS::query(Store& value) NOEXCEPT
  : store_(value)
{
}

// setters
// ----------------------------------------------------------------------------

// false: parent link, allocation.
TEMPLATE
bool CLASS::set_header(const system::chain::header& header,
    const context& context) NOEXCEPT
{
    // Bypass with success if header exists (by hash).
    if (store_.header.exists(header.hash()))
        return true;

    // Parent must be missing iff its hash is null.
    // Iterator must be released before subsequent header put.
    const auto& parent_sk = header.previous_block_hash();
    const auto parent_fk = store_.header.it(parent_sk).self();
    if (parent_fk.is_terminal() != (parent_sk == system::null_hash))
        return false;

    return !store_.header.put(header.hash(), table::header::record_put_ref
    {
        {},
        context,
        parent_fk,
        header
    }).is_terminal();
}

// false: tx empty, allocation.
TEMPLATE
bool CLASS::set_tx(const system::chain::transaction& tx) NOEXCEPT
{
    // Must have at least one input and output.
    if (tx.is_empty())
        return false;

    // Bypass with success if tx exists (by hash).
    if (store_.tx.exists(tx.hash(false)))
        return true;

    // Allocate one transaction.
    const auto tx_pk = store_.tx.allocate(1);
    if (tx_pk.is_terminal())
        return false;

    // Declare puts record.
    const auto& ins = *tx.inputs_ptr();
    const auto& outs = *tx.outputs_ptr();
    const auto count = outs.size() + ins.size();
    table::puts::record puts{};
    puts.put_fks.reserve(count);

    // Allocate and Set inputs, queue each put.
    uint32_t input_index = 0;
    for (const auto& in: ins)
    {
        const auto input_pk = store_.input.set(table::input::slab
        {
            {},
            tx_pk,
            input_index++,
            in->sequence(),
            in->script(),
            in->witness()
        });

        if (!input_pk.is_terminal())
            puts.put_fks.push_back(input_pk);
    }

    // Commit outputs, queue each put.
    uint32_t output_index = 0;
    for (const auto& out: outs)
    {
        const auto output_pk = store_.output.put(table::output::slab
        {
            {},
            tx_pk,
            output_index++,
            out->value(),
            out->script()
        });

        if (!output_pk.is_terminal())
            puts.put_fks.push_back(output_pk);
    }

    // Halt on error.
    if (puts.put_fks.size() != count)
        return false;

    // Commit puts (defined above).
    const auto puts_pk = store_.puts.put(puts);
    if (puts_pk.is_terminal())
        return false;

    // Set transaction.
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
        return false;
    }

    // Commit point and input for each input.
    const table::point::record point{};
    auto input_fk = puts.put_fks.begin();
    for (const auto& in: ins)
    {
        const auto& prevout = in->point();

        // Commit (empty) to prevout.hash (if missing).
        const auto point_pk = store_.point.put_if(prevout.hash(), point);
        if (point_pk.is_terminal())
            return false;

        // Commit each input_fk to its prevout fp.
        if (!store_.input.commit(*input_fk++, table::input::to_point(point_pk,
            prevout.index())))
            return false;
    }

    // Commit transaction to its hash.
    return store_.tx.commit(tx_pk, tx.hash(false));
}

// false: tx link, allocation.
TEMPLATE
bool CLASS::set_txs(const hash_digest& key, const system::hashes& hashes) NOEXCEPT
{
    table::txs::slab txs{};
    txs.tx_fks.reserve(hashes.size());

    // TODO: evaluate parallelization advantage (std::for_each).
    for (const auto& hash: hashes)
        txs.tx_fks.push_back(store_.tx.it(hash).self());

    // TODO: optmize contains using in-loop check.
    return !system::contains(txs.tx_fks, table::txs::link::terminal) &&
        !store_.txs.put(store_.header.it(key).self(), txs).is_terminal();
}

// false: allocation.
TEMPLATE
bool CLASS::set_block(const system::chain::block& block,
    const context& context) NOEXCEPT
{
    // TODO: evaluate parallelization advantage (std::for_each).
    for (const auto& tx : *block.transactions_ptr())
        if (!set_tx(*tx))
            return false;

    // TODO: optimize by returning header_pk for use in set_txs.
    return set_header(block.header(), context) &&
        set_txs(block.hash(), block.transaction_hashes(false));
}

// getters
// ----------------------------------------------------------------------------

// null: not found, unloaded.
TEMPLATE
system::chain::transaction::cptr CLASS::get_tx(const hash_digest& key) NOEXCEPT
{
    using namespace system::chain;

    table::transaction::record tx{};
    if (!store_.tx.get(key, tx))
        return {};

    table::puts::record inputs{};
    inputs.put_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.ins_fk, inputs))
        return {};

    table::puts::record outputs{};
    outputs.put_fks.resize(tx.outs_count);
    if (!store_.puts.get(tx.ins_fk + tx.ins_count, outputs))
        return {};

    // tx construct casts this inputs_ptr to an inputs_cptr.
    const auto ins = system::to_shared<system::chain::input_cptrs>();
    ins->reserve(tx.ins_count);
    table::point::record_sk pt{};
    table::input::slab_with_decomposed_sk in{};
    for (const auto& in_fk: inputs.put_fks)
    {
        if (!store_.input.get(in_fk, in) ||
            !store_.point.get(in.point_fk, pt))
            return {};

        // TODO: construct script/witness on ptr using custom deserializer.
        ins->emplace_back(new input
        {
            point{ std::move(pt.key), in.point_index },
            std::move(in.script),
            std::move(in.witness),
            in.sequence
        });
    }

    // tx construct casts this outputs_ptr to an outputs_cptr.
    const auto outs = system::to_shared<system::chain::output_cptrs>();
    outs->reserve(tx.outs_count);
    table::output::slab out{};
    for (const auto& out_fk: outputs.put_fks)
    {
        // TODO: construct script on ptr using custom deserializer.
        if (!store_.output.get(out_fk, out))
            return {};

        outs->emplace_back(new output
        {
            out.value,
            std::move(out.script)
        });
    }

    return system::to_shared(new transaction
    {
        tx.version,
        ins,
        outs,
        tx.locktime
    });
}

// null: not found, unloaded.
TEMPLATE
system::chain::header::cptr CLASS::get_header(const hash_digest& key) NOEXCEPT
{
    table::header::record header{};
    if (!store_.header.get(key, header))
        return {};

    // terminal (default) parent implies genesis, otherwise it must resolve.
    table::header::record_sk parent{};
    if ((header.parent_fk != table::header::link::terminal) &&
        !store_.header.get(header.parent_fk, parent))
        return {};

    return system::to_shared(new system::chain::header
    {
        header.version,
        std::move(parent.key),
        std::move(header.merkle_root),
        header.timestamp,
        header.bits,
        header.nonce
    });
}

// null: not found, unloaded.
TEMPLATE
system::hashes CLASS::get_txs(const hash_digest& key) NOEXCEPT
{
    return get_txs(store_.header.it(key).self());
}

// null: not found, unloaded.
TEMPLATE
system::hashes CLASS::get_txs(const table::header::link& header_fk) NOEXCEPT
{
    table::txs::slab txs{};
    if (!store_.txs.get(header_fk, txs))
        return {};

    system::hashes hashes{};
    hashes.reserve(txs.tx_fks.size());
    table::transaction::record_sk tx{};
    for (const auto& tx_fk: txs.tx_fks)
        if (store_.tx.get(tx_fk, tx))
            hashes.push_back(std::move(tx.key));

    if (hashes.size() != txs.tx_fks.size())
        return {};

    return hashes;
}

// null: not found, unloaded.
TEMPLATE
system::chain::block::cptr CLASS::get_block(const hash_digest& key) NOEXCEPT
{
    const auto header = get_header(key);
    if (!header)
        return {};

    // TODO: optmize by querying with header_fk and getting tx fks.
    const auto hashes = get_txs(key);
    if (hashes.empty())
        return {};

    // block construct casts this transactions_ptr to a transactions_cptr.
    const auto txs = system::to_shared<system::chain::transaction_ptrs>();
    txs->reserve(hashes.size());
    for (const auto& hash: hashes)
        txs->push_back(get_tx(hash));

    // TODO: optmize contains using in-loop check.
    if (system::contains(*txs, nullptr))
        return {};

    return system::to_shared(new system::chain::block
    {
        header,
        txs
    });
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
