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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WIRE_WRITER_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WIRE_WRITER_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// set transaction_view
// ----------------------------------------------------------------------------
// This is the only multitable write query (except initialize/genesis).
// The caller holds the transactor and all table accessors, with all table rows
// preallocated by the block writer. No allocation may occur under accessors.

TEMPLATE
code CLASS::set_code(std::vector<point>& twins, const accessors& ptrs,
    const allocation& fks, const transaction_view& tx, bool bypass) NOEXCEPT
{
    using namespace system;
    using ix = linkage<schema::index>;

    // View parser treats empty as invalid, so there is no empty state.
    // As a result this condition should never be hit (deserialization fails).
    if (!tx.is_valid())
        return error::tx_empty;

    const auto inputs = possible_narrow_cast<ix::integer>(tx.inputs());
    const auto outputs = possible_narrow_cast<ix::integer>(tx.outputs());
    const auto coinbase = tx.is_coinbase();

    // Contiguously store inputs (preallocated).
    if (!store_.input.put(ptrs.input, fks.in_fk,
        table::input::put_view{ {}, tx }))
        return error::tx_input_put;

    // Contiguously store outputs (preallocated).
    if (!store_.output.put(ptrs.output, fks.out_fk,
        table::output::put_view{ {}, fks.tx_fk, tx }))
        return error::tx_output_put;

    // Contiguously store input links (preallocated, rows shared with points).
    // Point elements are set into the same rows following tx set, below.
    // The caller's ins accessor guards raw sequence writes against remap.
    if (!store_.ins.sequence.put(fks.ins_fk,
        table::ins_sequence::put_view{ {}, fks.in_fk, fks.tx_fk, tx }))
        return error::tx_ins_put;

    // Contiguously store output links (preallocated).
    if (!store_.outs.put(ptrs.outs, fks.outs_fk,
        table::outs::put_view{ {}, fks.out_fk, tx }))
        return error::tx_outs_put;

    // Create tx record (preallocated).
    // Commit is deferred for point/address index consistency.
    if (!store_.tx.set(ptrs.tx, fks.tx_fk, tx.hash(false),
        table::transaction::put_view
        {
            {},
            tx,
            inputs,
            outputs,
            fks.ins_fk,
            fks.outs_fk
        }))
    {
        return error::tx_tx_set;
    }

    auto ins_fk = fks.ins_fk;
    auto ins = tx.get_inputs_stream();
    read::bytes::fast isource{ ins };

    // Commit points (hashmap).
    if (coinbase)
    {
        // Should only be one input, but generalized anyway.
        for (size_t in{}; in < inputs; ++in)
        {
            // Should always be a null point - but could be invalid.
            if (!store_.ins.put(ptrs.ins, ins_fk++, chain::point(isource),
                table::ins_point::record{}))
                return error::tx_null_point_put;

            // Skip script and sequence.
            isource.skip_bytes(isource.read_size());
            isource.skip_bytes(sizeof(uint32_t));
        }
    }
    else
    {
        if (store_.is_dirty() || !bypass)
        {
            // Collect duplicates for deferred store in duplicate table.
            for (size_t in{}; in < inputs; ++in)
            {
                bool duplicate{};
                if (!store_.ins.put(duplicate, ptrs.ins, ins_fk++,
                    chain::point(isource), table::ins_point::record{}))
                    return error::tx_point_put;

                if (duplicate)
                {
                    isource.rewind_bytes(chain::point::serialized_size());
                    twins.push_back(chain::point(isource));
                }

                // Skip script and sequence.
                isource.skip_bytes(isource.read_size());
                isource.skip_bytes(sizeof(uint32_t));
            }
        }
        else
        {
            for (size_t in{}; in < inputs; ++in)
            {
                if (!store_.ins.put(ptrs.ins, ins_fk++, chain::point(isource),
                    table::ins_point::record{}))
                    return error::tx_point_put;

                // Skip script and sequence.
                isource.skip_bytes(isource.read_size());
                isource.skip_bytes(sizeof(uint32_t));
            }
        }
    }

    BC_ASSERT(isource);
    if (!isource)
        return error::tx_null_point_put;

    // Commit address index records (hashmap, preallocated).
    if (address_enabled())
    {
        auto ad_fk = fks.ad_fk;
        auto out_fk = fks.out_fk;
        auto outs = tx.get_outputs_stream();
        read::bytes::fast osource{ outs };

        for (size_t out{}; out < outputs; ++out)
        {
            const auto value = osource.read_8_bytes_little_endian();
            const auto bytes = osource.read_size();

            if (!store_.address.put(ptrs.address, ad_fk++,
                sha256_hash(osource.read_bytes(bytes)),
                table::address::record{ {}, out_fk }))
                return error::tx_address_put;

            out_fk.value += possible_narrow_cast<output_link::integer>(
                tx_link::size + variable_size(value) + variable_size(bytes) +
                bytes);
        }

        BC_ASSERT(osource);
        if (!osource)
            return error::tx_address_put;
    }

    // Commit tx to search (hashmap).
    return store_.tx.commit(ptrs.tx, fks.tx_fk, tx.hash(false)) ?
        error::success : error::tx_tx_commit;
}

// set txs from block
// ----------------------------------------------------------------------------
// This sets only the txs of a block with header/context already archived.
// Block MUST be kept in scope until all transactions are written. ~block()
// releases all memory for parts of itself, due to the custom allocator.

TEMPLATE
code CLASS::set_code(const block_view& block, bool strong,
    bool bypass) NOEXCEPT
{
    header_link unused{};
    return set_code(unused, block, strong, bypass);
}

TEMPLATE
code CLASS::set_code(header_link& out_fk, const block_view& block, bool strong,
    bool bypass) NOEXCEPT
{
    out_fk = to_header(block.hash());
    if (out_fk.is_terminal())
        return error::txs_header;

    size_t height{};
    if (!get_height(height, out_fk))
        return error::txs_height;

    return set_code(block, out_fk, strong, bypass, height);
}

TEMPLATE
code CLASS::set_code(const block_view& block, const header_link& key,
    bool strong, bool bypass, size_t height) NOEXCEPT
{
    using namespace system;
    using in_t = input_link::integer;
    using out_t = output_link::integer;
    using ins_t = ins_link::integer;
    using outs_t = outs_link::integer;
    using address_t = address_link::integer;

    if (key.is_terminal())
        return error::txs_header;

    const auto txs = block.transactions();
    if (is_zero(txs))
        return error::txs_empty;

    // Sum full block allocation for each table from cached view metadata.
    size_t points{};
    size_t outputs{};
    size_t input_bytes{};
    size_t output_bytes{};
    for (const auto& tx: block.views())
    {
        points += tx.inputs();
        outputs += tx.outputs();
        input_bytes += tx.input_table_size();
        output_bytes += tx.output_table_size();
    }

    // Optional hash, only has value on height intervals.
    auto interval = create_interval(key, height);

    // Depth/forks only set by writer for genesis (is_zero(tx_fks[0])).
    const auto depth = store_.interval_depth();
    const auto forks = store_.fork_flags();

    using bytes = linkage<schema::size>::integer;
    const auto count = possible_narrow_cast<unsigned_type<schema::count_>>(txs);
    const auto light = possible_narrow_cast<bytes>(block.serialized_size(false));
    const auto heavy = possible_narrow_cast<bytes>(block.serialized_size(true));

    // ========================================================================
    const auto scope = get_transactor();

    // Allocate all block rows for each table (one allocation lock each).
    const auto tx_fks = store_.tx.allocate(count);
    if (tx_fks.is_terminal())
        return error::tx_tx_allocate;

    allocation fks{};
    fks.tx_fk = tx_fks;

    fks.in_fk = store_.input.allocate(
        possible_narrow_cast<in_t>(input_bytes));
    if (fks.in_fk.is_terminal())
        return error::tx_input_put;

    fks.out_fk = store_.output.allocate(
        possible_narrow_cast<out_t>(output_bytes));
    if (fks.out_fk.is_terminal())
        return error::tx_output_put;

    fks.ins_fk = store_.ins.allocate(
        possible_narrow_cast<ins_t>(points));
    if (fks.ins_fk.is_terminal())
        return error::tx_ins_put;

    fks.outs_fk = store_.outs.allocate(
        possible_narrow_cast<outs_t>(outputs));
    if (fks.outs_fk.is_terminal())
        return error::tx_outs_put;

    const auto address = address_enabled();
    if (address)
    {
        fks.ad_fk = store_.address.allocate(
            possible_narrow_cast<address_link::integer>(outputs));
        if (fks.ad_fk.is_terminal())
            return error::tx_address_allocate;
    }

    // Guard all tables against remap for the duration of the block write.
    // No table may be allocated while any of these accessors are held.
    accessors ptrs{};
    ptrs.tx = store_.tx.get_memory();
    ptrs.ins = store_.ins.get_memory();
    ptrs.outs = store_.outs.get_memory();
    ptrs.input  = store_.input.get_memory();
    ptrs.output = store_.output.get_memory();
    if (address)
        ptrs.address = store_.address.get_memory();

    if (!ptrs.input ||
        !ptrs.output ||
        !ptrs.ins ||
        !ptrs.outs ||
        !ptrs.tx ||
        (address && !ptrs.address))
        return error::unloaded_file;

    // Write all txs into their preallocated rows (write order preserved).
    code ec{};
    std::vector<point> twins{};
    for (const auto& tx: block.views())
    {
        if ((ec = set_code(twins, ptrs, fks, tx, bypass)))
            return ec;

        fks.tx_fk++;
        fks.ins_fk.value  += possible_narrow_cast<ins_t>(tx.inputs());
        fks.outs_fk.value += possible_narrow_cast<outs_t>(tx.outputs());
        fks.in_fk.value   += possible_narrow_cast<in_t>(tx.input_table_size());
        fks.out_fk.value  += possible_narrow_cast<out_t>(tx.output_table_size());
        fks.ad_fk.value   += possible_narrow_cast<address_t>(tx.outputs());
    }

    // Release all accessors (subsequent writes allocate).
    ptrs.tx.reset();
    ptrs.ins.reset();
    ptrs.outs.reset();
    ptrs.input.reset();
    ptrs.output.reset();
    ptrs.address.reset();

    // As few duplicates are expected, duplicate domain is only 2^16.
    // Return of tx_duplicate_put implies link domain has overflowed.
    for (const auto& twin: twins)
        if (!store_.duplicate.exists(twin))
            if (!store_.duplicate.put(twin, table::duplicate::record{}))
                return error::tx_duplicate_put;

    constexpr auto positive = true;

    // Transactor assures cannot be restored without txs, as required to unset.
    if (strong && !set_strong(key, txs, tx_fks, positive))
        return error::txs_confirm;

    // Header link is the key for the txs table.
    // Clean single allocation failure (e.g. disk full).
    return store_.txs.put(to_txs(key), table::txs::put_group
    {
        {},
        light,
        heavy,
        count,
        tx_fks,
        std::move(interval),
        depth,
        forks
    }) ? error::success : error::txs_txs_put;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
