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
#ifndef LIBBITCOIN_DATABASE_MEMORY_RELEASE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_RELEASE_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Head-release page-run helpers (pure, unit tested). Pages tracked as bits in
/// 64-bit words (low order bit is lowest page of word). Release candidate page
/// is clean (not dirty), cold (not recently written) and not already released.
/// Candidacy limited to full pages below logical page count.
/// ---------------------------------------------------------------------------

/// Candidate bits given page-state words.
constexpr uint64_t release_candidates(uint64_t dirty, uint64_t hot,
    uint64_t released) NOEXCEPT
{
    using namespace system;
    return bit_not(bit_or(dirty, bit_or(hot, released)));
}

/// Retain candidacy below page count, for word starting at page 'first' (full
/// word of candidacy above count clears to zero).
constexpr uint64_t release_below(uint64_t candidates, size_t pages,
    size_t first) NOEXCEPT
{
    using namespace system;
    constexpr auto bits = to_bits(sizeof(uint64_t));

    if (pages <= first)
        return zero;

    if (pages - first >= bits)
        return candidates;

    return bit_and(candidates, unmask_right<uint64_t>(pages - first));
}

/// Bits for pages [lo, hi) clamped to word of 64 pages starting at page
/// 'first', empty if ranges do not intersect.
constexpr uint64_t page_mask(size_t lo, size_t hi, size_t first) NOEXCEPT
{
    using namespace system;
    constexpr auto bits = to_bits(sizeof(uint64_t));
    const auto begin = std::max(lo, first);
    const auto end = std::min(hi, first + bits);

    if (begin >= end)
        return zero;

    const auto high = (end - first >= bits) ? bit_all<uint64_t> :
        unmask_right<uint64_t>(end - first);

    return bit_and(high, mask_right<uint64_t>(begin - first));
}

/// Maximal run of set bits at or above 'page' within 'pages', as right-open
/// page range [first, second), empty (equal) if none.
inline std::pair<size_t, size_t> next_run(const uint64_t* words, size_t pages,
    size_t page) NOEXCEPT
{
    using namespace system;
    constexpr auto bits = to_bits(sizeof(uint64_t));

    // Find first set bit at or above page.
    while ((page < pages) && !get_right(words[page / bits], page % bits))
        ++page;

    // Find first clear bit above start.
    auto end = page;
    while ((end < pages) && get_right(words[end / bits], end % bits))
        ++end;

    return { page, end };
}

/// Maximal run of set bits containing 'page' within 'pages', as right-open page
/// range [first, second), empty (equal) if page is not set.
inline std::pair<size_t, size_t> bit_run(const uint64_t* words, size_t pages,
    size_t page) NOEXCEPT
{
    using namespace system;
    constexpr auto bits = to_bits(sizeof(uint64_t));

    if ((page >= pages) || !get_right(words[page / bits], page % bits))
        return { page, page };

    auto start = page;
    while (!is_zero(start) &&
        get_right(words[sub1(start) / bits], sub1(start) % bits))
        --start;

    auto end = add1(page);
    while ((end < pages) && get_right(words[end / bits], end % bits))
        ++end;

    return { start, end };
}

} // namespace database
} // namespace libbitcoin

#endif
