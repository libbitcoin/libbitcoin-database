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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WRITE_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WRITE_IPP

#include <algorithm>
#include <ranges>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// bool returns
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set(const header& header, const chain_context& ctx,
    bool milestone) NOEXCEPT
{
    return !set_code(header, ctx, milestone);
}

TEMPLATE
bool CLASS::set(const header& header, const context& ctx, bool milestone) NOEXCEPT
{
    return !set_code(header, ctx, milestone);
}

TEMPLATE
bool CLASS::set(const block& block, const chain_context& ctx, bool milestone,
    bool strong) NOEXCEPT
{
    return !set_code(block, ctx, milestone, strong);
}

TEMPLATE
bool CLASS::set(const block& block, const context& ctx, bool milestone,
    bool strong) NOEXCEPT
{
    return !set_code(block, ctx, milestone, strong);
}

TEMPLATE
bool CLASS::set(const transaction& tx) NOEXCEPT
{
    return !set_code(tx);
}

TEMPLATE
bool CLASS::set(const block& block, bool strong, bool bypass,
    size_t height) NOEXCEPT
{
    // This sets only the txs of a block with header/context already archived.
    return !set_code(block, strong, bypass, height);
}

// set transaction
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::set_code(const transaction& tx) NOEXCEPT
{
    constexpr auto txs = system::possible_narrow_cast<tx_link::integer>(one);

    // Allocate tx record.
    const auto tx_fk = store_.tx.allocate(txs);
    if (tx_fk.is_terminal())
        return error::tx_tx_allocate;

    return set_code(tx_fk, tx, false);
}

TEMPLATE
code CLASS::set_code(const tx_link& tx_fk, const transaction& tx,
    bool bypass) NOEXCEPT
{
    // This is the only multitable write query (except initialize/genesis).

    if (tx.is_empty())
        return error::tx_empty;

    using namespace system;
    using ix = linkage<schema::index>;
    const auto& ins = tx.inputs_ptr();
    const auto& ous = tx.outputs_ptr();
    const auto inputs = possible_narrow_cast<ix::integer>(ins->size());
    const auto outputs = possible_narrow_cast<ix::integer>(ous->size());
    const auto coinbase = tx.is_coinbase();

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate contiguously and store inputs.
    input_link in_fk{};
    if (!store_.input.put_link(in_fk,
        table::input::put_ref{ {}, tx }))
        return error::tx_input_put;

    // Allocate contiguously and store outputs.
    output_link out_fk{};
    if (!store_.output.put_link(out_fk,
        table::output::put_ref{ {}, tx_fk, tx }))
        return error::tx_output_put;

    // Allocate and contiguously store input links.
    ins_link ins_fk{};
    if (!store_.ins.put_link(ins_fk,
        table::ins::put_ref{ {}, in_fk, tx_fk, tx }))
        return error::tx_ins_put;

    // Allocate and contiguously store output links.
    outs_link outs_fk{};
    if (!store_.outs.put_link(outs_fk,
        table::outs::put_ref{ {}, out_fk, tx }))
        return error::tx_outs_put;

    // Create tx record.
    // Commit is deferred for point/address index consistency.
    if (!store_.tx.set(tx_fk, tx.get_hash(false), table::transaction::put_ref
    {
        {},
        tx,
        inputs,
        outputs,
        ins_fk,
        outs_fk
    }))
    {
        return error::tx_tx_set;
    }

    // Commit points (hashmap).
    if (coinbase)
    {
        // Should only be one input, but generalized anyway.
        if (!store_.point.expand(ins_fk + inputs))
            return error::tx_point_allocate;

        for (const auto& in: *ins)
            if (!store_.point.put(ins_fk++, in->point(),
                table::point::record{}))
                return error::tx_null_point_put;
    }
    else
    {
        // Expand synchronizes keys with ins_fk, entries set into same offset.
        // Allocate contiguous points (at sequential keys matching ins_fk).
        if (!store_.point.expand(ins_fk + inputs))
            return error::tx_point_allocate;

        // If dirty we must guard against duplicates.
        // Dirty doesn't hold up in the case of an invalidated block, as that
        // may result in a duplicated tx. So dirty should be false in the case
        // of a non-bypass (valid) block.
        // Must be set after tx.set and before tx.commit, since searchable and
        // produces association to tx.link, and is also an integral part of tx.
        if (store_.is_dirty() || !bypass)
        {
            // Collect duplicates to store in duplicate table.
            std::vector<chain::point> twins{};
            auto ptr = store_.point.get_memory();
            for (const auto& in: *ins)
            {
                bool duplicate{};
                if (!store_.point.put(duplicate, ptr, ins_fk++, in->point(),
                    table::point::record{}))
                    return error::tx_point_put;

                if (duplicate)
                    twins.push_back(in->point());
            }

            ptr.reset();

            // As few duplicates are expected, duplicate domain is only 2^16.
            // Return of tx_duplicate_put implies link domain has overflowed.
            for (const auto& twin: twins)
                if (!store_.duplicate.exists(twin))
                    if (!store_.duplicate.put(twin, table::duplicate::record{}))
                        return error::tx_duplicate_put;
        }
        else
        {
            auto ptr = store_.point.get_memory();
            for (const auto& in: *ins)
                if (!store_.point.put(ptr, ins_fk++, in->point(),
                    table::point::record{}))
                    return error::tx_point_put;

            ptr.reset();
        }
    }

    // Commit address index records (hashmap).
    if (address_enabled())
    {
        auto ad_fk = store_.address.allocate(outputs);
        if (ad_fk.is_terminal())
            return error::tx_address_allocate;

        constexpr auto value_parent_diff = sizeof(uint64_t) - tx_link::size;
        const auto ptr = store_.address.get_memory();
        for (const auto& output: *ous)
        {
            if (!store_.address.put(ptr, ad_fk++, output->script().hash(),
                table::address::record{ {}, out_fk }))
                return error::tx_address_put;

            // See outs::put_ref.
            // Calculate next corresponding output fk from serialized size.
            // (variable_size(value) + (value + script)) - (value - parent)
            out_fk.value += (variable_size(output->value()) +
                output->serialized_size() - value_parent_diff);
        }
    }

    // Commit tx to search (hashmap).
    // tx.get_hash() assumes cached or is not thread safe.
    return store_.tx.commit(tx_fk, tx.get_hash(false)) ?
        error::success : error::tx_tx_commit;
    // ========================================================================
}

// set header
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::set_code(const header& header, const context& ctx,
    bool milestone) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, header, ctx, milestone);
}

TEMPLATE
code CLASS::set_code(const header& header, const chain_context& ctx,
    bool milestone) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, header, ctx, milestone);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const header& header,
    const chain_context& ctx, bool milestone, bool) NOEXCEPT
{
    // Map chain context into database context.
    return set_code(out_fk, header, context
    {
        system::possible_narrow_cast<context::flag_t::integer>(ctx.flags),
        system::possible_narrow_cast<context::height_t::integer>(ctx.height),
        ctx.median_time_past
    }, milestone);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const header& header,
    const context& ctx, bool milestone, bool) NOEXCEPT
{
    // header.get_hash() assumes cached or is not thread safe.
    const auto& key = header.get_hash();

    // Parent must be missing iff its hash is null.
    const auto& previous = header.previous_block_hash();
    const auto parent_fk = to_header(previous);
    if (parent_fk.is_terminal() != (previous == system::null_hash))
        return system::error::orphan_block;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    out_fk = store_.header.put_link(key, table::header::put_ref
    {
        {},
        ctx,
        milestone,
        parent_fk,
        header
    });

    return out_fk.is_terminal() ? error::header_put : error::success;
    // ========================================================================
}

// set full block
// ----------------------------------------------------------------------------
// strong is set for checkpointed blocks only, as they are always strong chain.

TEMPLATE
code CLASS::set_code(const block& block, const context& ctx, bool milestone,
    bool strong) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, ctx, milestone, strong);
}

TEMPLATE
code CLASS::set_code(const block& block, const chain_context& ctx,
    bool milestone, bool strong) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, ctx, milestone, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    const chain_context& ctx, bool milestone, bool strong) NOEXCEPT
{
    // Map chain context into database context.
    return set_code(out_fk, block, context
    {
        system::possible_narrow_cast<context::flag_t::integer>(ctx.flags),
        system::possible_narrow_cast<context::height_t::integer>(ctx.height),
        ctx.median_time_past
    }, milestone, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    const context& ctx, bool milestone, bool strong) NOEXCEPT
{
    const auto ec = set_code(out_fk, block.header(), ctx, milestone);
    return ec ? ec : set_code(block, out_fk, strong, strong || milestone,
        ctx.height);
}

// set txs from block
// ----------------------------------------------------------------------------
// This sets only the txs of a block with header/context already archived.
// Block MUST be kept in scope until all transactions are written. ~block()
// releases all memory for parts of itself, due to the custom allocator.

TEMPLATE
code CLASS::set_code(const block& block, bool strong, bool bypass,
    size_t height) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, strong, bypass, height);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block, bool strong,
    bool bypass, size_t height) NOEXCEPT
{
    out_fk = to_header(block.get_hash());
    if (out_fk.is_terminal())
        return error::txs_header;

    return set_code(block, out_fk, strong, bypass, height);
}

TEMPLATE
code CLASS::set_code(const block& block, const header_link& key,
    bool strong, bool bypass, size_t /* height */) NOEXCEPT
{
    using namespace system;
    if (key.is_terminal())
        return error::txs_header;

    const auto txs = block.transactions();
    if (is_zero(txs))
        return error::txs_empty;

    const auto count = possible_narrow_cast<tx_link::integer>(txs);
    const auto tx_fks = store_.tx.allocate(count);
    if (tx_fks.is_terminal())
        return error::tx_tx_allocate;

    code ec{};
    auto fk = tx_fks;
    for (const auto& tx: *block.transactions_ptr())
        if ((ec = set_code(fk++, *tx, bypass)))
            return ec;

    using bytes = linkage<schema::size>::integer;
    auto interval = get_interval(key, height);
    const auto size = block.serialized_size(true);
    const auto wire = possible_narrow_cast<bytes>(size);

    // ========================================================================
    const auto scope = store_.get_transactor();
    constexpr auto positive = true;

    // Transactor assures cannot be restored without txs, as required to unset.
    if (strong && !set_strong(key, txs, tx_fks, positive))
        return error::txs_confirm;

    // Header link is the key for the txs table.
    // Clean single allocation failure (e.g. disk full).
    return store_.txs.put(to_txs(key), table::txs::put_group
    {
        {},
        wire,
        count,
        tx_fks,
        std::move(interval)
    }) ? error::success : error::txs_txs_put;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
