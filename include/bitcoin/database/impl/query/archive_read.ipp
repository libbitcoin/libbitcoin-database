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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_READ_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_READ_IPP

#include <algorithm>
#include <ranges>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Bools.
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_header(const hash_digest& key) const NOEXCEPT
{
    return store_.header.exists(key);
}

TEMPLATE
inline bool CLASS::is_block(const hash_digest& key) const NOEXCEPT
{
    return is_associated(to_header(key));
}

TEMPLATE
inline bool CLASS::is_tx(const hash_digest& key) const NOEXCEPT
{
    return store_.tx.exists(key);
}

TEMPLATE
inline bool CLASS::is_coinbase(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_coinbase tx{};
    return store_.tx.get(link, tx) && tx.coinbase;
}

TEMPLATE
inline bool CLASS::is_milestone(const header_link& link) const NOEXCEPT
{
    table::header::get_milestone header{};
    return store_.header.get(link, header) && header.milestone;
}

TEMPLATE
inline bool CLASS::is_associated(const header_link& link) const NOEXCEPT
{
    table::txs::get_associated txs{};
    return store_.txs.find(link, txs) && txs.associated;
}

// Empty/null_hash implies fault, zero count implies unassociated.
// ----------------------------------------------------------------------------

TEMPLATE
hashes CLASS::get_tx_keys(const header_link& link) const NOEXCEPT
{
    const auto tx_fks = to_transactions(link);
    if (tx_fks.empty())
        return {};

    system::hashes hashes{};
    hashes.reserve(tx_fks.size());
    for (const auto& tx_fk: tx_fks)
        hashes.push_back(get_tx_key(tx_fk));

    // Return of any null_hash implies failure.
    return hashes;
}

TEMPLATE
size_t CLASS::get_tx_count(const header_link& link) const NOEXCEPT
{
    table::txs::get_tx_quantity txs{};
    if (!store_.txs.find(link, txs))
        return {};

    return txs.number;
}

TEMPLATE
inline hash_digest CLASS::get_header_key(const header_link& link) const NOEXCEPT
{
    return store_.header.get_key(link);
}

TEMPLATE
inline hash_digest CLASS::get_tx_key(const tx_link& link) const NOEXCEPT
{
    return store_.tx.get_key(link);
}

TEMPLATE
inline point_key CLASS::get_point_key(const point_link& link) const NOEXCEPT
{
    table::point::get_composed point{};
    if (!store_.point.get(link, point))
        return {};

    return point.key;
}

TEMPLATE
inline hash_digest CLASS::get_point_hash(const point_link& link) const NOEXCEPT
{
    table::point::record point{};
    if (!store_.point.get(link, point))
        return {};

    return point.hash;
}

// False implies not confirmed.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT
{
    // to_block is strong but not necessarily confirmed.
    const auto fk = to_block(link);
    return is_confirmed_block(fk) && get_height(out, fk);
}

TEMPLATE
bool CLASS::get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT
{
    // to_block is strong but not necessarily confirmed.
    const auto fk = to_block(link);
    if (!is_confirmed_block(fk))
        return false;

    // False return below implies an integrity error (tx should be indexed).
    table::txs::get_position txs{ {}, link };
    if (!store_.txs.find(fk, txs))
        return false;

    out = txs.position;
    return true;
}

TEMPLATE
bool CLASS::get_tx_sizes(size_t& light, size_t& heavy,
    const tx_link& link) const NOEXCEPT
{
    table::transaction::get_sizes sizes{};
    if (!store_.tx.get(link, sizes))
        return false;

    light = sizes.light;
    heavy = sizes.heavy;
    return true;
}

// Terminal implies not found, false implies fault.
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::get_block_size(const header_link& link) const NOEXCEPT
{
    table::txs::get_block_size txs{};
    return store_.txs.find(link, txs) ? txs.wire : zero;
}

TEMPLATE
height_link CLASS::get_height(const hash_digest& key) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.find(key, header))
        return {};

    return header.height;
}

TEMPLATE
height_link CLASS::get_height(const header_link& link) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.get(link, header))
        return {};

    return header.height;
}

TEMPLATE
bool CLASS::get_height(size_t& out, const hash_digest& key) const NOEXCEPT
{
    const auto height = get_height(key);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

TEMPLATE
bool CLASS::get_height(size_t& out, const header_link& link) const NOEXCEPT
{
    // Use get_height(..., key) in place of get(to_header(key)).
    const auto height = get_height(link);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

TEMPLATE
bool CLASS::get_value(uint64_t& out, const output_link& link) const NOEXCEPT
{
    table::output::get_value output{};
    if (!store_.output.get(link, output))
        return false;

    out = output.value;
    return true;
}

TEMPLATE
bool CLASS::get_unassociated(association& out,
    const header_link& link) const NOEXCEPT
{
    if (is_associated(link))
        return false;

    table::header::get_check_context context{};
    if (!store_.header.get(link, context))
        return false;

    out =
    {
        link,
        context.key,
        system::chain::context
        {
            context.ctx.flags,
            context.timestamp,
            context.ctx.mtp,
            system::possible_wide_cast<size_t>(context.ctx.height)
        }
    };

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
