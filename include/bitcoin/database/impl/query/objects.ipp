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
#ifndef LIBBITCOIN_DATABASE_QUERY_OBJECTS_IPP
#define LIBBITCOIN_DATABASE_QUERY_OBJECTS_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Chain objects.
// ----------------------------------------------------------------------------

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
typename CLASS::input::cptr CLASS::get_input(const tx_link& link,
    uint32_t input_index) const NOEXCEPT
{
    return get_input(to_spend(link, input_index));
}

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    return get_output(to_output(link, output_index));
}

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(const tx_link& link,
    uint32_t output_index) const NOEXCEPT
{
    return get_spenders(to_output(link, output_index));
}

// Populate prevout objects.
// ----------------------------------------------------------------------------

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

} // namespace database
} // namespace libbitcoin

#endif
