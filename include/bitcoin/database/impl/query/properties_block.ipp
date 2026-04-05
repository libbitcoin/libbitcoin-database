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
#ifndef LIBBITCOIN_DATABASE_QUERY_PROPERTIES_BLOCK_IPP
#define LIBBITCOIN_DATABASE_QUERY_PROPERTIES_BLOCK_IPP

#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// boolean
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
inline bool CLASS::is_block_segregated(const header_link& link) const NOEXCEPT
{
    size_t light{}, heavy{};
    return get_block_sizes(light, heavy, link) && heavy != light;
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
    return store_.txs.at(to_txs(link), txs) && txs.associated;
}

// properties
// ----------------------------------------------------------------------------

// node/is_current
TEMPLATE
uint32_t CLASS::get_top_timestamp(bool confirmed) const NOEXCEPT
{
    const auto top = confirmed ? to_confirmed(get_top_confirmed()) :
        to_candidate(get_top_candidate());

    // returns zero if read fails.
    uint32_t timestamp{};
    /* bool */ get_timestamp(timestamp, top);
    return timestamp;
}

TEMPLATE
bool CLASS::get_timestamp(uint32_t& timestamp,
    const header_link& link) const NOEXCEPT
{
    table::header::get_timestamp header{};
    if (!store_.header.get(link, header))
        return false;

    timestamp = header.timestamp;
    return true;
}

TEMPLATE
bool CLASS::get_version(uint32_t& version,
    const header_link& link) const NOEXCEPT
{
    table::header::get_version header{};
    if (!store_.header.get(link, header))
        return false;

    version = header.version;
    return true;
}

TEMPLATE
bool CLASS::get_work(uint256_t& work, const header_link& link) const NOEXCEPT
{
    uint32_t bits{};
    const auto result = get_bits(bits, link);
    work = header::proof(bits);
    return result;
}

TEMPLATE
bool CLASS::get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT
{
    table::header::get_bits header{};
    if (!store_.header.get(link, header))
        return false;

    bits = header.bits;
    return true;
}

TEMPLATE
bool CLASS::get_context(context& ctx, const header_link& link) const NOEXCEPT
{
    table::header::record_context header{};
    if (!store_.header.get(link, header))
        return false;

    ctx = std::move(header.ctx);
    return true;
}

TEMPLATE
bool CLASS::get_context(system::chain::context& ctx,
    const header_link& link) const NOEXCEPT
{
    table::header::record_context header{};
    if (!store_.header.get(link, header))
        return false;

    // Context for block/header.check and header.accept are filled from
    // chain_state, not from the store.
    ctx =
    {
        header.ctx.flags,     // [block.check, block.accept & block.connect]
        {},                   // [block.check] timestamp
        header.ctx.mtp,       // [block.check, header.accept]
        header.ctx.height,    // [block.check & block.accept]
        {},                   // [header.accept] minimum_block_version
        {}                    // [header.accept] work_required
    };

    return true;
}

// height
// ----------------------------------------------------------------------------

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
    const auto height = get_height(link);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

// association
// ----------------------------------------------------------------------------
// Empty/null_hash implies fault, zero count implies unassociated.

TEMPLATE
hashes CLASS::get_tx_keys(const header_link& link) const NOEXCEPT
{
    const auto tx_fks = to_transactions(link);
    if (tx_fks.empty())
        return {};

    // Overallocate as required for the common merkle scenario.
    const auto count = tx_fks.size();
    const auto size = is_odd(count) && !is_one(count) ? add1(count) : count;

    system::hashes hashes{};
    hashes.reserve(size);
    for (const auto& tx_fk: tx_fks)
        hashes.push_back(get_tx_key(tx_fk));

    // Return of any null_hash implies failure.
    return hashes;
}

TEMPLATE
size_t CLASS::get_tx_count(const header_link& link) const NOEXCEPT
{
    table::txs::get_tx_count txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return txs.number;
}

TEMPLATE
tx_link CLASS::get_position_tx(const header_link& link,
    size_t position) const NOEXCEPT
{
    table::txs::get_at_position txs{ {}, position };
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return txs.tx_fk;
}

} // namespace database
} // namespace libbitcoin

#endif
