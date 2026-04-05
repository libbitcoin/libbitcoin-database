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
#ifndef LIBBITCOIN_DATABASE_QUERY_LOCATOR_IPP
#define LIBBITCOIN_DATABASE_QUERY_LOCATOR_IPP

#include <algorithm>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// locator readers
// ----------------------------------------------------------------------------
// These do not require strict consistency.

// node/header-out
TEMPLATE
CLASS::headers CLASS::get_headers(const hashes& locator,
    const hash_digest& stop, size_t limit) const NOEXCEPT
{
    headers out{};
    const auto span = get_locator_span(locator, stop, limit);
    out.reserve(span.size());

    for (auto height = span.begin; height < span.end; ++height)
    {
        // Terminal implies intervening reorganization.
        const auto link = to_confirmed(height);
        if (link.is_terminal())
            return {};

        out.push_back(get_header(link));
        BC_ASSERT(!is_null(out.back()));
    }

    return out;
}

// node/block-out
TEMPLATE
hashes CLASS::get_blocks(const hashes& locator,
    const hash_digest& stop, size_t limit) const NOEXCEPT
{
    hashes out{};
    const auto span = get_locator_span(locator, stop, limit);
    out.reserve(span.size());

    for (auto height = span.begin; height < span.end; ++height)
    {
        // Terminal implies intervening reorganization.
        const auto link = to_confirmed(height);
        if (link.is_terminal())
            return {};

        out.push_back(get_header_key(link));
        BC_ASSERT(out.back() != system::null_hash);
    }

    return out;
}

// utilities
// ----------------------------------------------------------------------------
// protected

TEMPLATE
CLASS::span CLASS::get_locator_span(const hashes& locator,
    const hash_digest& stop, size_t limit) const NOEXCEPT
{
    using namespace system;

    // Start at fork point, stop at given header (both excluded).
    const auto start = add1(get_locator_start(locator));
    const auto last1 = (stop == null_hash) ? max_uint32 :
        get_height(to_header(stop)).value;

    // Determine number of headers requested, limited by max allowed.
    const auto request = floored_subtract<size_t>(last1, start);
    const auto allowed = std::min(request, limit);

    // Set end to (start + allowed), limited by (top + 1).
    const auto top1 = ceilinged_add(get_top_confirmed(), one);
    const auto end = std::min(ceilinged_add(start, allowed), top1);

    // max converts negative range to empty.
    return
    {
        start,
        std::max(start, end)
    };
}

TEMPLATE
size_t CLASS::get_locator_start(const hashes& locator) const NOEXCEPT
{
    // Locator is presumed (by convention) to be in reverse order by height.
    for (const auto& hash: locator)
    {
        const auto link = to_header(hash);
        if (link.is_terminal())
            continue;

        const auto height = get_height(link);
        table::height::record confirmed{};
        if (store_.confirmed.get(height, confirmed) &&
            confirmed.header_fk == link)
                return height;
    }

    return zero;
}

} // namespace database
} // namespace libbitcoin

#endif
