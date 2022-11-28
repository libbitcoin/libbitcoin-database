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

TEMPLATE
bool CLASS::set_header(const system::chain::header& header,
    const context& context) NOEXCEPT
{
    const auto& parent_sk = header.previous_block_hash();

    // Iterator must be released before subsequent header put.
    const auto parent_fk = store_.header.it(parent_sk).self();

    // Parent must be missing iff its hash is null.
    if ((parent_fk == table::header::link::terminal) != 
        (parent_sk == system::null_hash))
        return false;

    return store_.header.put(header.hash(), table::header::record
    {
        {},
        context.height,
        context.flags,
        context.mtp,
        parent_fk,
        header.version(),
        header.timestamp(),
        header.bits(),
        header.nonce(),
        header.merkle_root()
    });
}

TEMPLATE
bool CLASS::set_tx(const system::chain::transaction& tx) NOEXCEPT
{
    // Must have at least one input and output.
    if (tx.is_empty())
        return false;

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
    linkage<schema::put> input_pk{};
    for (const auto& in: ins)
    {
        if (store_.input.set_link(input_pk, table::input::slab
        {
            {},
            tx_pk,
            input_index++,
            in->sequence(),
            in->script(),
            in->witness()
        }))
        {
            puts.put_fks.push_back(input_pk);
        }
    }

    // Commit outputs, queue each put.
    uint32_t output_index = 0;
    linkage<schema::put> output_pk{};
    for (const auto& out: outs)
    {
        if (store_.output.put_link(output_pk, table::output::slab
        {
            {},
            tx_pk,
            output_index++,
            out->value(),
            out->script()
        }))
        {
            puts.put_fks.push_back(output_pk);
        }
    }

    // Halt on error.
    if (puts.put_fks.size() != count)
        return false;

    // Commit puts.
    ////table::puts::record puts{};
    linkage<schema::puts_> puts_pk{};
    if (!store_.puts.put_link(puts_pk, puts))
        return false;

    // Set transaction.
    // TODO: instead of weight store tx.serialized_size(true)?
    if (!store_.tx.set(tx_pk, table::transaction::record
    {
        {},
        tx.is_coinbase(),
        system::possible_narrow_cast<uint32_t>(tx.serialized_size(false)),
        system::possible_narrow_cast<uint32_t>(tx.weight()),
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
    auto input_fk = puts.put_fks.begin();
    linkage<schema::point::pk> point_pk{};
    const table::point::record empty{};
    for (const auto& in: ins)
    {
        const auto& prevout = in->point();

        // Commit (empty) to prevout.hash (if missing).
        if (!store_.point.put_if(point_pk, prevout.hash(), empty))
            return false;

        // Commit each input_fk to its prevout fp.
        if (!store_.input.commit(*input_fk++, table::input::to_point(point_pk,
            prevout.index())))
            return false;
    }

    // Commit transaction to its hash.
    return store_.tx.commit(tx_pk, tx.hash(false));
}

TEMPLATE
bool CLASS::set_block(const system::chain::block&) NOEXCEPT
{
    return false;
}

TEMPLATE
system::chain::header::cptr CLASS::get_header(const hash_digest& key) NOEXCEPT
{
    table::header::record element{};
    if (!store_.header.get(key, element))
        return {};

    // terminal parent implies genesis (default), otherwise must resolve.
    table::header::record_sk parent{};
    if ((element.parent_fk != table::header::link::terminal) &&
        !store_.header.get(element.parent_fk, parent))
        return {};

    // Use of pointer forward here avoids move construction.
    return system::to_shared(new system::chain::header
    {
        element.version,
        std::move(parent.key),
        std::move(element.root),
        element.timestamp,
        element.bits,
        element.nonce
    });
}

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

    input_cptrs ins{};
    ins.reserve(tx.ins_count);
    table::point::record_sk pt{};
    table::input::slab_with_decomposed_sk in{};
    for (const auto& in_fk: inputs.put_fks)
    {
        if (!store_.input.get(in_fk, in) ||
            !store_.point.get(in.point_fk, pt))
            return {};

        ins.emplace_back(new input
        {
            point{ pt.key, in.point_index },
            std::move(in.script),
            std::move(in.witness),
            in.sequence
        });
    }

    output_cptrs outs{};
    outs.reserve(tx.outs_count);
    table::output::slab out{};
    for (const auto& out_fk: outputs.put_fks)
    {
        if (!store_.output.get(out_fk, out))
            return {};

        outs.emplace_back(new output
        {
            out.value,
            std::move(out.script)
        });
    }

    return system::to_shared(new transaction
    {
        tx.version,
        system::to_shared(std::move(ins)),
        system::to_shared(std::move(outs)),
        tx.locktime
    });
}

TEMPLATE
system::chain::block::cptr CLASS::get_block(const hash_digest&) NOEXCEPT
{
    return {};
}

TEMPLATE
system::hashes CLASS::get_block_locator(const hash_digest&) NOEXCEPT
{
    return {};
}

TEMPLATE
system::hashes CLASS::get_block_txs(const hash_digest&) NOEXCEPT
{
    return {};
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
