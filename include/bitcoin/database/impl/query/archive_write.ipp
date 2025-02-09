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
code CLASS::set_code(tx_link& out_fk, const transaction& tx) NOEXCEPT
{
    // This is the only multitable write query (except initialize/genesis).

    using namespace system;
    if (tx.is_empty())
        return error::tx_empty;

    // tx.get_hash() assumes cached or is not thread safe.
    const auto& key = tx.get_hash(false);

    // Declare puts record.
    const auto& ins = tx.inputs_ptr();
    const auto& outs = tx.outputs_ptr();
    table::puts::slab puts{};
    puts.spend_fks.reserve(ins->size());
    puts.out_fks.reserve(outs->size());

    // Declare spends buffer.
    std::vector<spend_key> spends{};
    spends.reserve(ins->size());

    // TODO: eliminate shared memory pointer reallocations.
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate tx record.
    // Clean single allocation failure (e.g. disk full).
    out_fk = store_.tx.allocate(1);
    if (out_fk.is_terminal())
        return error::tx_tx_allocate;

    // Allocate spend records.
    // Clean single allocation failure (e.g. disk full).
    const auto count = possible_narrow_cast<spend_link::integer>(ins->size());
    auto spend_fk = store_.spend.allocate(count);
    if (spend_fk.is_terminal())
        return error::tx_spend_allocate;

    // Commit input records (spend records not indexed).
    for (const auto& in: *ins)
    {
        // Commit input record.
        // Safe allocation failure, blob linked by unindexed spend.
        input_link input_fk{};
        if (!store_.input.put_link(input_fk, table::input::put_ref
        {
            {},
            *in
        }))
        {
            return error::tx_input_put;
        }

        // Input point aliases.
        const auto& prevout = in->point();
        const auto& hash = prevout.hash();

        // Create prevout hash in point table.
        point_link hash_fk{};
        if (prevout.index() != chain::no_previous_output)
        {
            // GUARD (tx redundancy)
            // Only fully effective if there is a single database thread.
            // This reduces point store by ~45GiB, but causes thrashing.
            if (minimize_)
                hash_fk = to_point(hash);

            if (hash_fk.is_terminal())
            {
                // Safe allocation failure, duplicates limited but expected.
                if (!store_.point.put_link(hash_fk, hash, table::point::record
                {
                    // Table stores no data other than the search key.
                }))
                {
                    return error::tx_point_put;
                }
            }
        }

        // Accumulate spend keys in order (terminal for any null point).
        spends.push_back(table::spend::compose(hash, prevout.index()));

        // Write spend record.
        // Safe allocation failure, index is deferred because invalid tx_fk.
        if (!store_.spend.set(spend_fk, table::spend::record
        {
            {},
            hash_fk,
            out_fk,
            in->sequence(),
            input_fk
        }))
        {
            return error::tx_spend_set;
        }

        // Accumulate spends (input references) in order.
        puts.spend_fks.push_back(spend_fk.value++);
    }

    // Commit output records.
    for (const auto& out: *outs)
    {
        // Safe allocation failure, blob unlinked.
        output_link output_fk{};
        if (!store_.output.put_link(output_fk, table::output::put_ref
        {
            {},
            out_fk,
            *out
        }))
        {
            return error::tx_output_put;
        }

        // Accumulate outputs in order.
        puts.out_fks.push_back(output_fk);
    }

    // Commit accumulated puts.
    // Safe allocation failure, unlinked blob links spend/output blobs.
    const auto puts_fk = store_.puts.put_link(puts);
    if (puts_fk.is_terminal())
        return error::tx_puts_put;

    // Write tx record.
    // Safe allocation failure, index is deferred for spend index consistency.
    using ix = linkage<schema::index>;
    if (!store_.tx.set(out_fk, table::transaction::record_put_ref
    {
        {},
        tx,
        system::possible_narrow_cast<ix::integer>(ins->size()),
        system::possible_narrow_cast<ix::integer>(outs->size()),
        puts_fk
    }))
    {
        return error::tx_tx_set;
    }

    // Commit spends to search.
    // Safe allocation failure, unindexed txs linked by spend, others unlinked.
    // A replay of committed spends without indexed tx will appear as double
    // spends, but the spend cannot be confirmed without the indexed tx. Spends
    // without indexed txs should be suppressed by c/s interface query.
    for (const auto& spend: std::views::reverse(spends))
    {
        --spend_fk.value;
        if (store_.spend.commit_link(spend_fk, spend).is_terminal())
            return error::tx_spend_commit;
    }

    // Commit addresses to search if address index is enabled.
    if (address_enabled())
    {
        auto output_fk = puts.out_fks.begin();
        for (const auto& out: *outs)
        {
            // Safe allocation failure, unindexed tx outputs linked by address,
            // others unlinked. A replay of committed addresses without indexed
            // tx will appear as double spends, but the spend cannot be
            // confirmed without the indexed tx. Addresses without indexed txs
            // should be suppressed by c/s interface query.
            if (!store_.address.put(out->script().hash(), table::address::record
            {
                {},
                *output_fk++
            }))
            {
                return error::tx_address_put;
            }
        }
    }

    // Commit tx to search.
    // Clean single allocation failure (e.g. disk full).
    return store_.tx.commit(out_fk, key) ? error::success : error::tx_tx_commit;
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
