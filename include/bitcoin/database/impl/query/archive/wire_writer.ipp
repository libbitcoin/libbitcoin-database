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

TEMPLATE
code CLASS::set_code(const tx_link& tx_fk, const transaction_view& tx,
    bool bypass) NOEXCEPT
{
    using namespace system;
    using ix = linkage<schema::index>;

    if (tx.is_empty())
        return error::tx_empty;

    const auto inputs = possible_narrow_cast<ix::integer>(tx.inputs());
    const auto outputs = possible_narrow_cast<ix::integer>(tx.outputs());
    const auto coinbase = tx.is_coinbase();

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Allocate contiguously and store inputs.
    input_link in_fk{};
    if (!store_.input.put_link(in_fk,
        table::input::put_view{ {}, tx }))
        return error::tx_input_put;

    // Allocate contiguously and store outputs.
    output_link out_fk{};
    if (!store_.output.put_link(out_fk,
        table::output::put_view{ {}, tx_fk, tx }))
        return error::tx_output_put;

    // Allocate and contiguously store input links.
    ins_link ins_fk{};
    if (!store_.ins.put_link(ins_fk,
        table::ins::put_view{ {}, in_fk, tx_fk, tx }))
        return error::tx_ins_put;

    // Allocate and contiguously store output links.
    outs_link outs_fk{};
    if (!store_.outs.put_link(outs_fk,
        table::outs::put_view{ {}, out_fk, tx }))
        return error::tx_outs_put;

    // Create tx record.
    // Commit is deferred for point/address index consistency.
    if (!store_.tx.set(tx_fk, tx.hash(false), table::transaction::put_view
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

        auto source = tx.get_inputs_stream();
        read::bytes::fast ins{ source };
        for (size_t in{}; in < inputs; ++in)
        {
            // Should always be a null point - but could be invalid.
            if (!store_.point.put(ins_fk++, chain::point(ins),
                table::point::record{}))
                return error::tx_null_point_put;

            // Skip script.
            ins.skip_bytes(ins.read_size());
        }
    }
    else
    {
        // Expand synchronizes keys with ins_fk, entries set into same offset.
        // Allocate contiguous points (at sequential keys matching ins_fk).
        if (!store_.point.expand(ins_fk + inputs))
            return error::tx_point_allocate;

        if (store_.is_dirty() || !bypass)
        {
            // Collect duplicates to store in duplicate table.
            std::vector<chain::point> twins{};
            auto ptr = store_.point.get_memory();
            auto source = tx.get_inputs_stream();
            read::bytes::fast ins{ source };
            for (size_t in{}; in < inputs; ++in)
            {
                bool duplicate{};
                if (!store_.point.put(duplicate, ptr, ins_fk++,
                    chain::point(ins), table::point::record{}))
                    return error::tx_point_put;
            
                if (duplicate)
                {
                    ins.rewind_bytes(chain::point::serialized_size());
                    twins.push_back(chain::point(ins));
                }

                // Skip script.
                ins.skip_bytes(ins.read_size());
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
            auto source = tx.get_inputs_stream();
            read::bytes::fast ins{ source };
            for (size_t in{}; in < inputs; ++in)
            {
                if (!store_.point.put(ptr, ins_fk++, chain::point(ins),
                    table::point::record{}))
                    return error::tx_point_put;

                // Skip script.
                ins.skip_bytes(ins.read_size());
            }

            ptr.reset();
        }
    }

    // Commit address index records (hashmap).
    if (address_enabled())
    {
        auto ad_fk = store_.address.allocate(outputs);
        if (ad_fk.is_terminal())
            return error::tx_address_allocate;

        constexpr auto value_parent = sizeof(uint64_t) - tx_link::size;
        const auto ptr = store_.address.get_memory();
        auto source = tx.get_outputs_stream();
        read::bytes::fast ous{ source };
        for (size_t out{}; out < outputs; ++out)
        {
            const auto start = ous.get_read_position();
            const auto value = ous.read_variable();
            if (!store_.address.put(ptr, ad_fk++,
                sha256_hash(ous.read_bytes(ous.read_size())),
                table::address::record{ {}, out_fk }))
                return error::tx_address_put;

            // See outs::put_ref.
            // Calculate next corresponding output fk from serialized size.
            // (variable_size(value) + (value + script)) - (value - parent)
            const auto output_size = ous.get_read_position() - start;
            out_fk.value += (variable_size(value) + output_size - value_parent);
        }
    }

    // Commit tx to search (hashmap).
    return store_.tx.commit(tx_fk, tx.hash(false)) ?
        error::success : error::tx_tx_commit;
    // ========================================================================
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
    if (key.is_terminal())
        return error::txs_header;

    const auto txs = block.transactions();
    if (is_zero(txs))
        return error::txs_empty;

    const auto count = possible_narrow_cast<unsigned_type<schema::count_>>(txs);
    const auto tx_fks = store_.tx.allocate(count);
    if (tx_fks.is_terminal())
        return error::tx_tx_allocate;

    code ec{};
    auto fk = tx_fks;
    for (const auto& tx: block.views())
        if ((ec = set_code(fk++, tx, bypass)))
            return ec;

    // Optional hash, only has value on height intervals.
    auto interval = create_interval(key, height);

    // Depth is only set by writer for genesis (is_zero(tx_fks[0])).
    const auto depth = store_.interval_depth();

    using bytes = linkage<schema::size>::integer;
    const auto light = possible_narrow_cast<bytes>(
        block.serialized_size(false));
    const auto heavy = possible_narrow_cast<bytes>
        (block.serialized_size(true));

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
        light,
        heavy,
        count,
        tx_fks,
        std::move(interval),
        depth
    }) ? error::success : error::txs_txs_put;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
