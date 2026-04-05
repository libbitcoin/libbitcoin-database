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
#ifndef LIBBITCOIN_DATABASE_QUERY_SIZES_IPP
#define LIBBITCOIN_DATABASE_QUERY_SIZES_IPP

#include <atomic>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// block/tx heavy/light sizes (heavy is nominal, light is witnessed).
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

// tx/block virtual sizes (combines heavy and light via consensus weighting).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_tx_virtual_size(size_t& out,
    const tx_link& link) const NOEXCEPT
{
    size_t light{}, heavy{};
    if (!get_tx_sizes(light, heavy, link))
        return false;

    out = system::chain::virtual_size(light, heavy);
    return true;
}

TEMPLATE
bool CLASS::get_block_virtual_size(size_t& out,
    const header_link& link) const NOEXCEPT
{
    size_t light{}, heavy{};
    if (!get_block_sizes(light, heavy, link))
        return false;

    out = system::chain::virtual_size(light, heavy);
    return true;
}

// Chain 
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::get_candidate_size() const NOEXCEPT
{
    // If the store is not opened this will be a max_size loop.
    return get_candidate_size(get_top_candidate());
}

TEMPLATE
size_t CLASS::get_candidate_size(size_t top) const NOEXCEPT
{
    size_t wire{};
    for (auto height = zero; height <= top; ++height)
    {
        size_t size{};
        if (!get_block_size(size, to_candidate(height), true))
            return max_size_t;

        wire += size;
    }

    return wire;
}

TEMPLATE
size_t CLASS::get_confirmed_size() const NOEXCEPT
{
    // If the store is not opened this will be a max_size loop.
    return get_confirmed_size(get_top_confirmed());
}

TEMPLATE
size_t CLASS::get_confirmed_size(size_t top) const NOEXCEPT
{
    size_t wire{};
    for (auto height = zero; height <= top; ++height)
    {
        size_t size{};
        if (!get_block_size(size, to_confirmed(height), true))
            return max_size_t;

        wire += size;
    }

    return wire;
}

} // namespace database
} // namespace libbitcoin

#endif
