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
bool CLASS::set(const block& block, bool strong) NOEXCEPT
{
    // This sets only the txs of a block with header/context already archived.
    return !set_code(block, strong);
}

// set transaction
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::set_code(const transaction& tx) NOEXCEPT
{
    tx_link unused{};
    return set_code(unused, tx);
}

TEMPLATE
code CLASS::set_code(tx_link& tx_fk, const transaction& tx) NOEXCEPT
{
    // This is the only multitable write query (except initialize/genesis).
    using namespace system;
    if (tx.is_empty())
        return error::tx_empty;

    const auto& ins = tx.inputs_ptr();
    const auto& outs = tx.outputs_ptr();

    using ix = linkage<schema::index>;
    const auto txs = possible_narrow_cast<tx_link::integer>(one);
    const auto inputs = possible_narrow_cast<ix::integer>(ins->size());
    const auto outputs = possible_narrow_cast<ix::integer>(outs->size());

    // Declare puts record for output accumulation.
    table::puts::record puts{};
    puts.out_fks.reserve(outputs);

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate single tx record.
    tx_fk = store_.tx.allocate(txs);
    if (tx_fk.is_terminal())
        return error::tx_tx_allocate;

    // Allocate points records.
    const auto point_fk = store_.point.allocate(inputs);
    auto point_it = point_fk;
    if (point_fk.is_terminal())
        return error::tx_point_allocate;

    // Commit input and point records.
    for (const auto& in: *ins)
    {
        input_link input_fk{};
        if (!store_.input.put_link(input_fk, table::input::put_ref
        {
            {},
            *in
        }))
        {
            return error::tx_input_put;
        }

        if (!store_.point.put(point_it++, table::point::record
        {
            {},
            in->point().hash(),
            in->point().index(),
            in->sequence(),
            input_fk,
            tx_fk
        }))
        {
            return error::tx_point_put;
        }
    }

    // Commit output records.
    for (const auto& out: *outs)
    {
        output_link output_fk{};
        if (!store_.output.put_link(output_fk, table::output::put_ref
        {
            {},
            tx_fk,
            *out
        }))
        {
            return error::tx_output_put;
        }

        // Accumulate outputs in order.
        puts.out_fks.push_back(output_fk);
    }

    // Commit accumulated puts.
    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return error::tx_puts_put;

    // Create tx record.
    // Commit is deferred for spend/address index consistency.
    if (!store_.tx.set(tx_fk, table::transaction::record_put_ref
    {
        {},
        tx,
        inputs,
        outputs,
        point_fk,
        puts_fk
    }))
    {
        return error::tx_tx_set;
    }

    // Commit spend index records.
    if (!tx.is_coinbase())
    {
        auto sp_fk = store_.spend.allocate(inputs);
        if (sp_fk.is_terminal())
            return error::tx_spend_allocate;

        auto in = ins->begin();
        const auto ptr = store_.spend.get_memory();

        for (auto pt_fk = point_fk; pt_fk < (point_fk + inputs); ++pt_fk)
        {
            const auto key = table::spend::compose((*in++)->point());
            if (!store_.spend.put(ptr, sp_fk++, key, table::spend::record
            {
                {},
                pt_fk
            }))
            {
                return error::tx_spend_put;
            }
        }
    }

    // Commit address index records.
    if (address_enabled())
    {
        auto ad_fk = store_.address.allocate(outputs);
        if (ad_fk.is_terminal())
            return error::tx_address_allocate;

        auto out = outs->begin();
        const auto ptr = store_.address.get_memory();

        for (auto out_fk: puts.out_fks)
        {
            const auto key = (*out++)->script().hash();
            if (!store_.address.put(ptr, ad_fk++, key, table::address::record
            {
                {},
                out_fk
            }))
            {
                return error::tx_address_put;
            }
        }
    }

    // Commit tx to search.
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
        system::possible_narrow_cast<context::flag::integer>(ctx.flags),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
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
    out_fk = store_.header.put_link(key, table::header::record_put_ref
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
        system::possible_narrow_cast<context::flag::integer>(ctx.flags),
        system::possible_narrow_cast<context::block::integer>(ctx.height),
        ctx.median_time_past
    }, milestone, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    const context& ctx, bool milestone, bool strong) NOEXCEPT
{
    const auto ec = set_code(out_fk, block.header(), ctx, milestone);
    return ec ? ec : set_code(block, out_fk, strong);
}

// set txs from block
// ----------------------------------------------------------------------------
// This sets only the txs of a block with header/context already archived.
// Block MUST be kept in scope until all transactions are written. ~block()
// releases all memory for parts of itself, due to the custom allocator.

TEMPLATE
code CLASS::set_code(const block& block, bool strong) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, strong);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block& block,
    bool strong) NOEXCEPT
{
    out_fk = to_header(block.get_hash());
    if (out_fk.is_terminal())
        return error::txs_header;

    return set_code(block, out_fk, strong);
}

TEMPLATE
code CLASS::set_code(const block& block, const header_link& key,
    bool strong) NOEXCEPT
{
    txs_link unused{};
    return set_code(unused, block, key, strong);
}

TEMPLATE
code CLASS::set_code(txs_link& out_fk, const block& block,
    const header_link& key, bool strong) NOEXCEPT
{
    if (key.is_terminal())
        return error::txs_header;

    const auto count = block.transactions();
    if (is_zero(count))
        return error::txs_empty;

    code ec{};
    tx_link tx_fk{};
    tx_links links{};
    links.reserve(count);
    for (const auto& tx: *block.transactions_ptr())
    {
        // Each tx is set under a distinct transactor.
        if ((ec = set_code(tx_fk, *tx)))
            return ec;

        links.push_back(tx_fk.value);
    }

    using bytes = linkage<schema::size>::integer;
    const auto size = block.serialized_size(true);
    const auto wire = system::possible_narrow_cast<bytes>(size);

    // ========================================================================
    const auto scope = store_.get_transactor();
    constexpr auto positive = true;

    // Clean allocation failure (e.g. disk full), see set_strong() comments.
    // Transactor assures cannot be restored without txs, as required to unset.
    if (strong && !set_strong(key, links, positive))
        return error::txs_confirm;

    // Header link is the key for the txs table.
    // Clean single allocation failure (e.g. disk full).
    out_fk = store_.txs.put_link(key, table::txs::slab
    {
        {},
        wire,
        links
    });

    return out_fk.is_terminal() ? error::txs_txs_put : error::success;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
