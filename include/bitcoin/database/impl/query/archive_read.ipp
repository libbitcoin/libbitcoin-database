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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_READ_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_READ_IPP

#include <atomic>
#include <numeric>
#include <utility>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>

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
inline bool CLASS::is_tx_segregated(const tx_link& link) const NOEXCEPT
{
    size_t light{}, heavy{};
    return get_tx_sizes(light, heavy, link) && heavy != light;
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

TEMPLATE
inline bool CLASS::is_confirmable(const header_link& link) const NOEXCEPT
{
    return get_header_state(link) == error::block_confirmable;
}

// Empty/null_hash implies fault, zero count implies unassociated.
// ----------------------------------------------------------------------------

TEMPLATE
hash_digest CLASS::get_top_confirmed_hash() const NOEXCEPT
{
    return get_header_key(to_confirmed(get_top_confirmed()));
}

TEMPLATE
hash_digest CLASS::get_top_candidate_hash() const NOEXCEPT
{
    return get_header_key(to_candidate(get_top_candidate()));
}

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

// Position.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT
{
    const auto fk = to_strong(link);
    return is_confirmed_block(fk) && get_height(out, fk);
}

TEMPLATE
bool CLASS::get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT
{
    const auto fk = to_strong(link);
    if (!is_confirmed_block(fk))
        return false;

    // False return below implies an integrity error (tx should be indexed).
    table::txs::get_position txs{ {}, link };
    if (!store_.txs.at(to_txs(fk), txs))
        return false;

    out = txs.position;
    return true;
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

// Sizes.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_tx_size(size_t& out, const tx_link& link,
    bool witness) const NOEXCEPT
{
    size_t light{}, heavy{};
    if (!get_tx_sizes(light, heavy, link))
        return false;
    
    out = witness ? heavy : light;
    return true;
}

TEMPLATE
bool CLASS::get_block_size(size_t& out, const header_link& link,
    bool witness) const NOEXCEPT
{
    size_t light{}, heavy{};
    if (!get_block_sizes(light, heavy, link))
        return false;

    out = witness ? heavy : light;
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

TEMPLATE
bool CLASS::get_block_sizes(size_t& light, size_t& heavy,
    const header_link& link) const NOEXCEPT
{
    table::txs::get_sizes sizes{};
    if (!store_.txs.at(to_txs(link), sizes))
        return false;

    light = sizes.light;
    heavy = sizes.heavy;
    return true;
}

// Heights.
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

// Values (value, spend, fees).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_value(uint64_t& out, const output_link& link) const NOEXCEPT
{
    table::output::get_value output{};
    if (!store_.output.get(link, output))
        return false;

    out = output.value;
    return true;
}

// protected
TEMPLATE
bool CLASS::get_outputs_total_value(uint64_t& out,
    const output_links& links) const NOEXCEPT
{
    out = zero;
    for (const auto& output_fk: links)
    {
        uint64_t value{};
        if (!get_value(value, output_fk)) return false;
        out = system::ceilinged_add(out, value);
    }

    return true;
}

TEMPLATE
bool CLASS::get_tx_value(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    table::transaction::get_coinbase tx{};
    if (!store_.tx.get(link, tx))
        return false;

    // Shortcircuit coinbase prevout read.
    if (tx.coinbase)
    {
        out = zero;
        return true;
    }

    // Optimizable due to sequential tx input links.
    const auto links = to_prevouts(link);
    return !links.empty() && get_outputs_total_value(out, links);
}

TEMPLATE
bool CLASS::get_tx_spend(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    const auto links = to_outputs(link);
    return !links.empty() && get_outputs_total_value(out, links);
}

TEMPLATE
bool CLASS::get_tx_fee(uint64_t& out, const tx_link& link) const NOEXCEPT
{
    uint64_t value{};
    if (!get_tx_value(value, link))
        return false;

    // Zero input implies either zero output or coinbase (both zero).
    if (is_zero(value))
        return true;

    uint64_t spend{};
    if (!get_tx_spend(spend, link) || spend > value)
        return false;

    out = value - spend;
    return true;
}

TEMPLATE
bool CLASS::get_block_value(uint64_t& out,
    const header_link& link) const NOEXCEPT
{
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || (txs.tx_fks.size() < one))
        return false;

    std::atomic_bool fail{};
    const auto begin = std::next(txs.tx_fks.begin());
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    out = std::transform_reduce(parallel, begin, txs.tx_fks.end(), 0_u64,
        [](uint64_t left, uint64_t right) NOEXCEPT
        {
            return system::ceilinged_add(left, right);
        },
        [&](const auto& tx_fk) NOEXCEPT
        {
            uint64_t value{};
            if (!fail.load(relaxed) && !get_tx_value(value, tx_fk))
                fail.store(true, relaxed);

            return value;
        });

    return !fail.load(relaxed);
}

TEMPLATE
bool CLASS::get_block_spend(uint64_t& out,
    const header_link& link) const NOEXCEPT
{
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || (txs.tx_fks.size() < one))
        return false;

    std::atomic_bool fail{};
    const auto begin = std::next(txs.tx_fks.begin());
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    out = std::transform_reduce(parallel, begin, txs.tx_fks.end(), 0_u64,
        [](uint64_t left, uint64_t right) NOEXCEPT
        {
            return system::ceilinged_add(left, right);
        },
        [&](const auto& tx_fk) NOEXCEPT
        {
            uint64_t spend{};
            if (!fail.load(relaxed) && !get_tx_spend(spend, tx_fk))
                fail.store(true, relaxed);

            return spend;
        });

    return !fail.load(relaxed);
}

TEMPLATE
bool CLASS::get_block_fee(uint64_t& out,
    const header_link& link) const NOEXCEPT
{
    uint64_t value{}, spend{};
    if (!get_block_value(value, link) || !get_block_spend(spend, link) ||
        spend > value)
        return false;

    out = value - spend;
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
