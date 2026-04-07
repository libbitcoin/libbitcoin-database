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
#ifndef LIBBITCOIN_DATABASE_QUERY_NAVIGATE_REVERSE_IPP
#define LIBBITCOIN_DATABASE_QUERY_NAVIGATE_REVERSE_IPP

#include <algorithm>
#include <atomic>
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

TEMPLATE
code CLASS::to_address_outputs(output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    // Pushing into the vector is more efficient than precomputation of size.
    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        out.push_back(address.output_fk);
    }

    return error::success;
}

TEMPLATE
code CLASS::to_address_outputs(std::atomic_bool& cancel, output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    // Pushing into the vector is more efficient than precomputation of size.
    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        out.push_back(address.output_fk);
    }

    return error::success;
}

// input|output|prevout->tx[parent]
// ----------------------------------------------------------------------------

TEMPLATE
tx_link CLASS::to_spending_tx(const point_link& link) const NOEXCEPT
{
    table::ins::get_parent ins{};
    if (!store_.ins.get(link, ins))
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
tx_link CLASS::to_prevout_tx(const point_link& link) const NOEXCEPT
{
    return to_tx(get_point_hash(link));
}

// output->inputs[spenders]
// ----------------------------------------------------------------------------
// get_spenders(point) is deduped, but to_spenders(output) is not, it returns
// links to ALL spenders of the output including duplicates and double spends.
// This allows result to be navigated via to_block() to find any strong spend.

TEMPLATE
point_links CLASS::to_spenders(const output_link& link) const NOEXCEPT
{
    table::output::get_parent out{};
    if (!store_.output.get(link, out))
        return {};

    // This results in two reads to the tx table, so could be optimized.
    return to_spenders(out.parent_fk, to_output_index(out.parent_fk, link));
}

TEMPLATE
point_links CLASS::to_spenders(const tx_link& output_tx,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders(get_tx_key(output_tx), output_index);
}

TEMPLATE
point_links CLASS::to_spenders(const hash_digest& point_hash,
    uint32_t output_index) const NOEXCEPT
{
    return to_spenders({ point_hash, output_index });
}

TEMPLATE
point_links CLASS::to_spenders(const point& point) const NOEXCEPT
{
    // Avoid returning spend links for coinbase inputs (not spenders).
    if (point.index() == point::null_index)
        return {};

    point_links points{};
    for (auto it = store_.point.it(point); it; ++it)
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
    const point_link& point_fk) const NOEXCEPT
{
    uint32_t index{};
    for (const auto& in_fk : to_points(parent_fk))
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
    for (const auto& out_fk : to_outputs(parent_fk))
    {
        if (out_fk == output_fk) return index;
        ++index;
    }

    return point::null_index;
}

} // namespace database
} // namespace libbitcoin

#endif
