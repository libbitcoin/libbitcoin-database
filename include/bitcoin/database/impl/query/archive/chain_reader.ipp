/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_CHAIN_READER_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_CHAIN_READER_IPP

#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// tx_link->inputs|outputs
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_inputs(
    const tx_link& link, bool witness) const NOEXCEPT
{
    // TODO: eliminate shared memory pointer reallocations.
    using namespace system;
    const auto fks = to_points(link);
    if (fks.empty())
        return {};

    const auto inputs = to_shared<chain::input_cptrs>();
    inputs->reserve(fks.size());

    for (const auto& fk: fks)
        if (!push_bool(*inputs, get_input(fk, witness)))
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

// header_link->txs
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::transactions_ptr CLASS::get_transactions(
    const header_link& link, bool witness) const NOEXCEPT
{
    // TODO: eliminate shared memory pointer reallocations.
    using namespace system;
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    const auto transactions = to_shared<chain::transaction_cptrs>();
    transactions->reserve(txs.size());

    for (const auto& tx_fk: txs)
        if (!push_bool(*transactions, get_transaction(tx_fk, witness)))
            return {};

    return transactions;
}

// header_link->header
// ----------------------------------------------------------------------------

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

// header_link->block
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::block::cptr CLASS::get_block(const header_link& link,
    bool witness) const NOEXCEPT
{
    const auto header = get_header(link);
    if (!header)
        return {};

    const auto transactions = get_transactions(link, witness);
    if (!transactions)
        return {};

    return system::to_shared<block>
    (
        header,
        transactions
    );
}

// tx_link->tx
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::transaction::cptr CLASS::get_transaction(const tx_link& link,
    bool witness) const NOEXCEPT
{
    using namespace system;
    table::transaction::only_with_sk tx{};
    if (!store_.tx.get(link, tx))
        return {};

    table::outs::record outs{};
    outs.out_fks.resize(tx.outs_count);
    if (!store_.outs.get(tx.outs_fk, outs))
        return {};

    const auto inputs = to_shared<chain::input_cptrs>();
    const auto outputs = to_shared<chain::output_cptrs>();
    inputs->reserve(tx.ins_count);
    outputs->reserve(tx.outs_count);

    // Points are allocated contiguously.
    for (auto fk = tx.point_fk; fk < (tx.point_fk + tx.ins_count); ++fk)
        if (!push_bool(*inputs, get_input(fk, witness)))
            return {};

    for (const auto& fk: outs.out_fks)
        if (!push_bool(*outputs, get_output(fk)))
            return {};

    const auto ptr = to_shared<transaction>
    (
        tx.version,
        inputs,
        outputs,
        tx.locktime
    );

    // TODO: store caches sizes so these could be forwarded.
    // Witness hash is not retained by the store.
    ptr->set_nominal_hash(std::move(tx.key));
    return ptr;
}

// point_link->point
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::point CLASS::get_point(
    const point_link& link) const NOEXCEPT
{
    table::point::record point{};
    if (!store_.point.get(link, point))
        return {};

    return { point.hash, point.index };
}

// point_link->witness
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::witness::cptr CLASS::get_witness(
    const point_link& link) const NOEXCEPT
{
    table::input::get_witness in{};
    table::ins::get_input ins{};
    if (!store_.ins.get(link, ins) ||
        !store_.input.get(ins.input_fk, in))
        return {};

    return in.witness;
}

// point_link->input_script
// ----------------------------------------------------------------------------
TEMPLATE
typename CLASS::script::cptr CLASS::get_input_script(
    const point_link& link) const NOEXCEPT
{
    table::input::get_script in{};
    table::ins::get_input ins{};
    if (!store_.ins.get(link, ins) ||
        !store_.input.get(ins.input_fk, in))
        return {};

    return in.script;
}

// [tx_link, index]->input
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const tx_link& link,
    uint32_t index, bool witness) const NOEXCEPT
{
    return get_input(to_point(link, index), witness);
}

// point_link->input
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::input::cptr CLASS::get_input(const point_link& link,
    bool witness) const NOEXCEPT
{
    using namespace system;
    table::input::get_ptrs in{ {}, witness };
    table::ins::get_input ins{};
    table::point::record point{};
    if (!store_.ins.get(link, ins) ||
        !store_.point.get(link, point) ||
        !store_.input.get(ins.input_fk, in))
        return {};

    const auto ptr = to_shared<input>
    (
        make_point(std::move(point.hash), point.index),
        in.script,
        in.witness,
        ins.sequence
    );

    // Node only (cheap so always include).
    // Internally-populated points will have default link of max_uint32.
    ptr->metadata.point_link = link;
    return ptr;
}

// output_link->output_script
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::script::cptr CLASS::get_output_script(
    const output_link& link) const NOEXCEPT
{
    table::output::get_script out{};
    if (!store_.output.get(link, out))
        return {};

    return out.script;
}

// [tx_link, index]->output
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(const tx_link& link,
    uint32_t index) const NOEXCEPT
{
    return get_output(to_output(link, index));
}

// output_link->output
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::output::cptr CLASS::get_output(
    const output_link& link) const NOEXCEPT
{
    table::output::only out{};
    if (!store_.output.get(link, out))
        return {};

    return out.output;
}

// output_link->inputs[spenders]
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::inputs_ptr CLASS::get_spenders(
    const output_link& link, bool witness) const NOEXCEPT
{
    using namespace system;
    const auto point_fks = to_spenders(link);
    const auto inputs = to_shared<chain::input_cptrs>();
    inputs->reserve(point_fks.size());

    for (const auto& point_fk: point_fks)
        if (!push_bool(*inputs, get_input(point_fk, witness)))
            return {};

    return inputs;
}

// output_link_->outpoint, point_link->inpoint, point->inpoints
// ----------------------------------------------------------------------------

TEMPLATE
outpoint CLASS::get_outpoint(const output_link& link) const NOEXCEPT
{
    table::output::get_parent_value out{};
    if (!store_.output.get(link, out))
        return {};

    const auto index = to_output_index(out.parent_fk, link);
    if (index == point::null_index)
        return {};

    return { { get_tx_key(out.parent_fk), index }, out.value };
}

TEMPLATE
inpoint CLASS::get_spender(const point_link& link) const NOEXCEPT
{
    const auto tx_fk = to_input_tx(link);
    if (tx_fk.is_terminal())
        return {};

    const auto index = to_input_index(tx_fk, link);
    if (index == point::null_index)
        return {};

    return { get_tx_key(tx_fk), index };
}

TEMPLATE
inpoints CLASS::get_spenders(const point& point) const NOEXCEPT
{
    inpoints ins{};
    for (const auto& point_fk: to_spenders(point))
        ins.insert(get_spender(point_fk));

    // std::set (lexically sorted/deduped).
    return ins;
}

// utility
// ----------------------------------------------------------------------------
// private/static


TEMPLATE
template <typename Bool>
inline bool CLASS::push_bool(std_vector<Bool>& stack,
    const Bool& element) NOEXCEPT
{
    if (!element)
        return false;

    stack.push_back(element);
    return true;
}

TEMPLATE
typename CLASS::point::cptr CLASS::make_point(hash_digest&& hash,
    uint32_t index) NOEXCEPT
{
    // Share null point instances to reduce memory consumption.
    static const auto null_point = system::to_shared<const point>();
    if (index == point::null_index)
        return null_point;

    return system::to_shared<point>(std::move(hash), index);
}

} // namespace database
} // namespace libbitcoin

#endif
