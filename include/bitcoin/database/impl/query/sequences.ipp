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
#ifndef LIBBITCOIN_DATABASE_QUERY_SEQUENCES_IPP
#define LIBBITCOIN_DATABASE_QUERY_SEQUENCES_IPP

#include <algorithm>
#include <ranges>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// protected hash/header sequence readers.
// ----------------------------------------------------------------------------
// Height-based readers are protected by reorg mutex (low contention).
// Reverse-navigation readers are inherently reorg safe (a little slower).

// node/block-in, node/header-in
TEMPLATE
hashes CLASS::get_candidate_hashes(const heights& heights) const NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ candidate_reorganization_mutex_ };

    for (const auto& height: heights)
    {
        const auto link = to_candidate(height);
        if (!link.is_terminal())
            out.push_back(get_header_key(link));
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

// unused
TEMPLATE
hashes CLASS::get_confirmed_hashes(const heights& heights) const NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ confirmed_reorganization_mutex_ };

    for (const auto& height: heights)
    {
        const auto link = to_confirmed(height);
        if (!link.is_terminal())
            out.push_back(get_header_key(link));
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

// unused
TEMPLATE
hashes CLASS::get_confirmed_hashes(size_t first, size_t count) const NOEXCEPT
{
    using namespace system;
    const auto size = count + to_int<size_t>(!is_one(count) && is_odd(count));
    if (is_zero(count) ||
        is_add_overflow(count, one) ||
        is_add_overflow(first, size))
        return {};

    auto link = to_confirmed(first + sub1(count));
    if (link.is_terminal())
        return {};

    // Extra allocation for odd count optimizes for merkle root.
    // Vector capacity is never reduced when resizing to smaller size.
    hashes out(size);
    out.resize(count);

    for (auto& hash: std::views::reverse(out))
    {
        hash = get_header_key(link);
        link = to_parent(link);
    }

    return out;
}

// server/electrum
TEMPLATE
header_links CLASS::get_confirmed_headers(size_t first,
    size_t limit) const NOEXCEPT
{
    // Empty is always a successful/valid result for this method.
    if (is_zero(limit))
        return {};

    // First requested height is currently above top.
    const auto top = get_top_confirmed();
    if (first > top)
        return {};

    // add1(top) cannot overflow, as indexed block maximum cannot exceed size_t.
    limit = std::min(limit, add1(top - first));
    auto last = sub1(first + limit);

    // Due to reorganization it is possible for this height to now be terminal.
    auto link = to_confirmed(last);

    // Walk link back to first indexed header (for reorg safety).
    while (link.is_terminal() && last > first)
        link = to_confirmed(--last);

    // No headers are currently confirmed at/above first.
    if (link.is_terminal())
        return {};

    // Compiler should optimize out last to_parent() call.
    header_links out(add1(last - first));
    for (auto& value: std::views::reverse(out))
        link = to_parent(value = link);

    return out;
}

// node filter-out
TEMPLATE
bool CLASS::get_ancestry(header_links& ancestry, const header_link& descendant,
    size_t count) const NOEXCEPT
{
    size_t height{};
    if (!get_height(height, descendant))
        return false;

    // Limit to genesis.
    count = std::min(add1(height), count);
    ancestry.resize(count);
    auto link = descendant;

    // Ancestry navigation ensures continuity without locks.
    // link terminal if previous was genesis (avoided by count <= height).
    for (auto& ancestor : ancestry)
        link = to_parent((ancestor = link));

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
