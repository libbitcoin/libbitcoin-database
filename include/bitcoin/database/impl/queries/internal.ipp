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

// fk setters(chain_ref)
// ----------------------------------------------------------------------------

TEMPLATE
table::header::link CLASS::set_header_(const system::chain::header& header,
    const context& context) NOEXCEPT
{
    // Bypass with success if header exists.
    const auto key = header.hash();
    auto header_fk = store_.header.it(key).self();
    if (!header_fk.is_terminal())
        return header_fk;

    // Parent must be missing iff its hash is null.
    // Iterator must be released before subsequent header put.
    const auto& parent_sk = header.previous_block_hash();
    const auto parent_fk = store_.header.it(parent_sk).self();
    if (parent_fk.is_terminal() != (parent_sk == system::null_hash))
        return {};

    return store_.header.put(key, table::header::record_put_ref
    {
        {},
        context,
        parent_fk,
        header
    });
}

TEMPLATE
table::transaction::link CLASS::set_tx_(
    const system::chain::transaction& tx) NOEXCEPT
{
    // Must have at least one input and output.
    if (tx.is_empty())
        return {};

    // Bypass with success if tx exists.
    const auto key = tx.hash(false);
    auto tx_pk = store_.tx.it(key).self();
    if (!tx_pk.is_terminal())
        return tx_pk;

    // Allocate one transaction.
    tx_pk = store_.tx.allocate(1);
    if (tx_pk.is_terminal())
        return {};

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
        return {};

    // Commit puts (defined above).
    const auto puts_pk = store_.puts.put(puts);
    if (puts_pk.is_terminal())
        return {};

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
        return {};
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
            return {};

        // Commit each input_fk to its prevout fp.
        if (!store_.input.commit(*input_fk++, table::input::to_point(point_pk,
            prevout.index())))
            return {};
    }

    // Commit transaction to its hash.
    return store_.tx.commit(tx_pk, key) ? tx_pk :
        table::transaction::link{};
}

TEMPLATE
table::header::link CLASS::set_block_(const system::chain::block& block,
    const context& context) NOEXCEPT
{
    const auto header_fk = set_header_(block.header(), context);
    if (header_fk.is_terminal())
        return {};

    // Get foreign key for each tx.
    table::txs::slab txs{};
    txs.tx_fks.reserve((*block.transactions_ptr()).size());
    for (const auto& tx: *block.transactions_ptr())
        txs.tx_fks.push_back(set_tx_(*tx));

    return set_txs_(header_fk, txs) ? header_fk : table::header::link{};
}

TEMPLATE
bool CLASS::set_txs_(const table::header::link& key,
    const table::txs::slab& txs) NOEXCEPT
{
    return !system::contains(txs.tx_fks, table::txs::link::terminal) &&
        !store_.txs.put(key, txs).is_terminal();
}

// chain_ptr getters(fk)
// ----------------------------------------------------------------------------

TEMPLATE
system::chain::transaction::cptr CLASS::get_tx(
    const table::transaction::link& fk) NOEXCEPT
{
    using namespace system::chain;

    table::transaction::record tx{};
    if (!store_.tx.get(fk, tx))
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
system::chain::header::cptr CLASS::get_header(const table::header::link& fk) NOEXCEPT
{
    table::header::record header{};
    if (!store_.header.get(fk, header))
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
system::chain::block::cptr CLASS::get_block(const table::header::link& fk) NOEXCEPT
{
    const auto header = get_header(fk);
    if (!header)
        return {};

    // TODO: optmize by querying with header_fk and getting tx fks.
    const auto hashes = get_txs(fk);
    if (hashes.empty())
        return {};

    // block construct casts this transactions_ptr to a transactions_cptr.
    const auto txs = system::to_shared<system::chain::transaction_ptrs>();
    txs->reserve(hashes.size());
    for (const auto& hash: hashes)
        txs->push_back(get_tx(hash));

    if (system::contains(*txs, nullptr))
        return {};

    return system::to_shared(new system::chain::block
    {
        header,
        txs
    });
}

// null: not found, unloaded.
TEMPLATE
system::hashes CLASS::get_txs(const table::header::link& fk) NOEXCEPT
{
    table::txs::slab txs{};
    if (!store_.txs.get(fk, txs))
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

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
