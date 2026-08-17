/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_QUERY_NAVIGATE_REVERSE_IPP
#define LIBBITCOIN_DATABASE_QUERY_NAVIGATE_REVERSE_IPP

#include <algorithm>
#include <atomic>
#include <ranges>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// block->block[parent]
// ----------------------------------------------------------------------------
// Presumed to guaranee consistency as headers are not duplicated by the node.

TEMPLATE
header_link CLASS::to_parent(const header_link& link) const NOEXCEPT
{
    table::header::get_parent_fk header{};
    if (!store_.header.get(link, header))
        return {};

    // Terminal implies genesis (no parent).
    return header.parent_fk;
}

// address->outputs[receivers]
// ----------------------------------------------------------------------------
// There can be multiple spenders of the same output (due to conflicts) and
// multiple instances of an output under distinct links (due to tx dups).

TEMPLATE
code CLASS::to_touched_txs(tx_links& out,
    const output_links& outputs) const NOEXCEPT
{
    const stopper cancel{};
    return to_touched_txs(cancel, out, outputs);
}

TEMPLATE
code CLASS::to_touched_txs(const stopper& cancel, tx_links& out,
    const output_links& outputs) const NOEXCEPT
{
    // Reserve one for each output tx and one for spending input tx (estimate).
    out.clear();
    out.reserve(two * outputs.size());

    // Orders are reversed due to expected tx_links reversal, for faster sort.
    for (const auto& output: std::views::reverse(outputs))
    {
        if (cancel)
            return error::query_canceled;

        if (const auto tx = to_output_tx(output); tx.is_terminal())
            return error::integrity;
        else
            out.push_back(tx);
        
        for (const auto& input: std::views::reverse(to_spenders(output)))
        {
            if (const auto tx = to_input_tx(input); tx.is_terminal())
                return error::integrity;
            else
                out.push_back(tx);
        }
    }

    return error::success;
}

TEMPLATE
code CLASS::to_address_outputs(output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    const stopper cancel{};
    return to_address_outputs(cancel, out, key);
}

TEMPLATE
code CLASS::to_address_outputs(const stopper& cancel, output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    outs_link cursor{};
    return to_address_outputs(cancel, cursor, out, key, max_size_t);
}

TEMPLATE
code CLASS::to_address_outputs(const stopper& cancel, outs_link& cursor,
    output_links& out, const hash_digest& key, size_t limit) const NOEXCEPT
{
    out.clear();

    // Limit bounds candidates, verified following iterator disposal.
    // The iterator guards the aggregate, so puts is read unguarded.
    code deferred{ error::success };
    output_links candidates{};
    auto found = cursor.is_terminal();
    const auto end = cursor;
    auto it = store_.outs.it({ key });
    for (cursor = it.get(); it; ++it)
    {
        if (cancel)
            return error::query_canceled;

        if (it.get() == end)
        {
            found = true;
            break;
        }

        if (is_zero(limit--))
        {
            deferred = error::depth_limited;
            break;
        }

        table::outs::get_output put{};
        if (!store_.outs.puts.get_raw(*it, put))
            return error::integrity;

        candidates.push_back(put.out_fk);
    }

    it.reset();
    if (!deferred && !found)
        deferred = error::invalid_cursor;

    // Verify candidates by hashing their scripts in place.
    const auto ptr = store_.output.get_memory();
    table::output::match_script_hash output{ {}, key };
    for (const auto& candidate: candidates)
    {
        if (cancel)
            return error::query_canceled;

        if (!store_.output.raw(ptr, candidate, output))
            return error::integrity;

        if (output.match)
            out.push_back(candidate);
    }

    return deferred;
}

// input|output|prevout->tx[parent]
// ----------------------------------------------------------------------------

TEMPLATE
tx_link CLASS::to_input_tx(const ins_link& link) const NOEXCEPT
{
    table::ins_sequence::get_parent ins{};
    if (!store_.ins.sequence.get(link, ins))
        return {};

    return ins.parent_fk;
}

TEMPLATE
tx_link CLASS::to_output_tx(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    return out.parent_fk;
}

TEMPLATE
tx_link CLASS::to_prevout_tx(const ins_link& link) const NOEXCEPT
{
    return to_tx(get_point_hash(link));
}

// output->inputs[spenders]
// ----------------------------------------------------------------------------
// get_spenders(point) is deduped, but to_spenders(output) is not, it returns
// links to ALL spenders of the output including duplicates and double spends.
// This allows result to be navigated via to_block() to find any strong spend.

TEMPLATE
ins_links CLASS::to_spenders(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    // This results in two reads to the tx table, so could be optimized.
    return to_spenders(out.parent_fk, to_output_index(out.parent_fk, link));
}

TEMPLATE
ins_links CLASS::to_spenders(const tx_link& output_tx,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders(get_tx_key(output_tx), output_index);
}

TEMPLATE
ins_links CLASS::to_spenders(const hash_digest& point_hash,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders({ point_hash, output_index });
}

TEMPLATE
ins_links CLASS::to_spenders(const point& point) const NOEXCEPT
{
    // Avoid returning spend links for coinbase inputs (not spenders).
    if (point.is_null())
        return {};

    ins_links points{};
    for (auto it = store_.ins.it(point); it; ++it)
        points.push_back(*it);

    return points;
}

// tx.hash->txs (all instances of same tx by hash)
// ----------------------------------------------------------------------------

TEMPLATE
tx_links CLASS::to_duplicates(const hash_digest& tx_hash) const NOEXCEPT
{
    tx_links out{};
    for (auto it = store_.tx.it(tx_hash); it; ++it)
        out.push_back(*it);

    return out;
}

// tx->block[strong]
// ----------------------------------------------------------------------------
// protected (logically hazardous)

// to_block() is ONLY the association from tx link to its associating block.
// This will be terminal when link is not of the associated instance of the
// "same" transaction. Tx links in the txs association are always consistent
// with the strong association, as strong is set using txs links.

TEMPLATE
header_link CLASS::to_block(const tx_link& link) const NOEXCEPT
{
    table::strong_tx::record strong{};
    if (!store_.strong_tx.find(link, strong) || !strong.positive())
        return {};

    return strong.header_fk();
}

// utilities
// ----------------------------------------------------------------------------
// protected (presumed to not be externally useful)

TEMPLATE
uint32_t CLASS::to_input_index(const tx_link& parent_fk,
    const ins_link& point_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& in_fk: to_points(parent_fk))
    {
        if (in_fk == point_fk) return index;
        ++index;
    }

    return point::null_index;
}

TEMPLATE
uint32_t CLASS::to_output_index(const tx_link& parent_fk,
    const output_link& output_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& out_fk: to_outputs(parent_fk))
    {
        if (out_fk == output_fk) return index;
        ++index;
    }

    return point::null_index;
}

} // namespace database
} // namespace libbitcoin

#endif
