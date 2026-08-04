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
#ifndef LIBBITCOIN_DATABASE_MEMORY_UTILITIES_HPP
#define LIBBITCOIN_DATABASE_MEMORY_UTILITIES_HPP

#include <algorithm>
#include <atomic>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// The byte size of system pages, zero if failed.
BCD_API size_t page_size() NOEXCEPT;

/// The bytes of physical memory, zero if failed.
BCD_API uint64_t system_memory() NOEXCEPT;

/// The bytes of unused physical memory, zero if failed. Scarcity precedes
/// pressure: clean file cache is reclaimable, so the pressure level does not
/// raise while free memory exhausts.
BCD_API uint64_t system_free() NOEXCEPT;

/// The bytes of memory available to allocation without swapping, zero if
/// failed. Unlike free this includes reclaimable file cache, so it is the
/// appropriate scarcity signal (free approximates zero on a busy system).
BCD_API uint64_t system_available() NOEXCEPT;

/// The system memory pressure level (1 normal, 2 warning, 4 critical), zero
/// if failed or the platform provides no source.
BCD_API size_t system_pressure() NOEXCEPT;

/// The bytes held by system memory compression (a pressure precursor), zero
/// if failed or the platform provides no source.
BCD_API uint64_t system_compressed() NOEXCEPT;

/// Head-release page-run helpers (pure, unit tested). Pages are tracked as
/// bits in 64-bit words (low order bit is the lowest page of the word). A
/// release candidate page is clean (not dirty), cold (not recently written)
/// and not already released; candidacy is limited to full pages below the
/// logical page count.
/// ---------------------------------------------------------------------------

/// Candidate bits given page-state words.
constexpr uint64_t release_candidates(uint64_t dirty, uint64_t hot,
    uint64_t released) NOEXCEPT
{
    using namespace system;
    return bit_not(bit_or(dirty, bit_or(hot, released)));
}

/// Retain candidacy below the page count, for the word starting at page
/// 'first' (a full word of candidacy above the count clears to zero).
constexpr uint64_t release_below(uint64_t candidates, size_t pages,
    size_t first) NOEXCEPT
{
    using namespace system;
    return (pages <= first) ? zero :
        (pages - first >= to_bits(sizeof(uint64_t))) ? candidates :
            bit_and(candidates, unmask_right<uint64_t>(pages - first));
}

/// Bits for pages [lo, hi) clamped to the word of 64 pages starting at page
/// 'first', empty if the ranges do not intersect.
constexpr uint64_t page_mask(size_t lo, size_t hi, size_t first) NOEXCEPT
{
    using namespace system;
    constexpr auto bits = to_bits(sizeof(uint64_t));
    const auto begin = std::max(lo, first);
    const auto end = std::min(hi, first + bits);
    return (begin >= end) ? zero : bit_and(
        (end - first >= bits) ? bit_all<uint64_t> :
            unmask_right<uint64_t>(end - first),
        mask_right<uint64_t>(begin - first));
}

/// The maximal run of set bits at or above 'page' within 'pages', as the
/// right-open page range [first, second), empty (equal) if none.
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

/// The maximal run of set bits containing 'page' within 'pages', as the
/// right-open page range [first, second), empty (equal) if page is not set.
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

/// C++26: std::atomic<size_t>::fetch_max
template <typename Integral, if_integral_integer<Integral> = true>
Integral fetch_max(std::atomic<Integral>& atomic, Integral value) NOEXCEPT
{
    constexpr auto relaxed = std::memory_order_relaxed;

    auto atom = atomic.load(relaxed);
    while (value > atom && !atomic.compare_exchange_weak(atom, value, relaxed))
    {
    }

    return atom;
}

} // namespace database
} // namespace libbitcoin

#endif
