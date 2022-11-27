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
#ifndef LIBBITCOIN_DATABASE_STORE_QUERY_IPP
#define LIBBITCOIN_DATABASE_STORE_QUERY_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::query(store& value) NOEXCEPT
  : store_(value)
{
}

TEMPLATE
bool CLASS::set_header(const system::chain::header& header,
    const context& context) NOEXCEPT
{
    BC_ASSERT(header.is_valid());

    return store_.header.put(header.hash(), table::header::record
    {
        {},
        context.height,
        context.flags,
        context.mtp,
        store_.header.it(header.previous_block_hash()).self(),
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
    BC_ASSERT(tx.is_valid());

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
    for (auto& in: ins)
    {
        linkage<schema::put> input_pk{};
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
    for (auto& out: outs)
    {
        linkage<schema::put> output_pk{};
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
    if (!store_.puts.put(puts))
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
            system::possible_narrow_cast<uint32_t>(puts.put_fks.front()) // ???
        }))
    {
        return false;
    }

    // Commit point and input for each input.
    auto input_fk = puts.put_fks.begin();
    for (auto& in: ins)
    {
        const auto& prevout = in->point();

        // Commit (empty to) prevout.hash if missing (multiple txs/ins may ref).
        linkage<schema::point::pk> pk;
        if (!store_.point.put_if(pk, prevout.hash(), table::point::record{}))
            return false;

        // Commit each input_fk to its prevout fp.
        const auto fp = table::input::to_point(pk, prevout.index());
        if (!store_.input.commit(*input_fk++, fp))
            return false;
    }

    // Commit transaction to its hash.
    return store_.tx.commit(tx_pk, tx.hash(Witness));
}

TEMPLATE
bool CLASS::set_block(const system::chain::block& block) NOEXCEPT
{
    return false;
}

TEMPLATE
system::chain::header::cptr get_header(const hash_digest& key) NOEXCEPT
{
    return nullptr;
}

TEMPLATE
system::chain::transaction::cptr get_tx(const hash_digest& key) NOEXCEPT
{
    return nullptr;
}

TEMPLATE
system::chain::block::cptr get_block(const hash_digest& key) NOEXCEPT
{
    return nullptr;
}

TEMPLATE
system::hashes get_block_locator(const hash_digest& key) NOEXCEPT
{
    return nullptr;
}

TEMPLATE
system::hashes get_block_txs(const hash_digest& key) NOEXCEPT
{
    return nullptr;
}

} // namespace database
} // namespace libbitcoin

#endif
