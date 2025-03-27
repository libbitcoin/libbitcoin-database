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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_SIEVE_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_SIEVE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

// [------------------sieve-----------][--------------link--------------]
// [[select][--------screens---------]][--------------link--------------]

// [[111][1111111111111111111111111111]] empty/default
// [[000][1111111111111111111111111111]] 1 screen
// [[001][2222222222222211111111111111]] 2 screens
// [[010][3333333333222222222111111111]] 3 screens
// [[011][4444444333333322222221111111]] 4 screens
// [[100][5555554444443333332222211111]] 5 screens
// [[101][6666655555444443333322221111]] 6 screens
// [[110][7777666655554444333322221111]] 7 screens
// [[111][-888777766665555444333222111]] 8 screens (high order sentinel bit)
// [[111][1000000000000000000000000000]] saturated

// To minimize computation in alignment the sieve is defined as two dimensional
// table of precomputed masks. One dimension is determined by the screen
// selector and other is used to iterate through masks for the selected screen.
// A single screen requires one mask, a double two, and so on. The max selector
// value indicates that the first bit of the screen is a sentinel. This is set
// for either terminal (all other bits set) or overflow (no other bits set).
// If the sentinel is unset then the max screen is in used on remaining bits.
// The seive should be terminal if and only if the link is terminal, and when
// overflowed implies that the bucket is saturated, rendering it unscreened.

namespace libbitcoin {
namespace database {

// Suppress bogus warnings to use constexpr when function is consteval.
BC_PUSH_WARNING(USE_CONSTEXPR_FOR_FUNCTION)
BC_PUSH_WARNING(NO_ARRAY_INDEXING)

TEMPLATE
constexpr CLASS::sieve() NOEXCEPT
  : sieve(empty)
{
}

TEMPLATE
constexpr CLASS::sieve(type value) NOEXCEPT
  : sieve_{ value }
{
}

TEMPLATE
constexpr CLASS::type CLASS::value() const NOEXCEPT
{
    return sieve_;
}

TEMPLATE
constexpr CLASS::operator CLASS::type() const NOEXCEPT
{
    return sieve_;
}

// protected
TEMPLATE
constexpr CLASS::type CLASS::masks(size_t row, size_t column) const NOEXCEPT
{
    BC_ASSERT(column <= row);

    // Read/write member compressed array as if it was a two-dimesional array.
    constexpr auto get_offset = generate_offsets();
    return masks_[get_offset[row] + column];
}

TEMPLATE
constexpr bool CLASS::screened(type fingerprint) const NOEXCEPT
{
    if constexpr (disabled)
    {
        return true;
    }
    else
    {
        using namespace system;
        const auto row = shift_right(sieve_, screen_bits);
        if (row == limit)
        {
            if (sieve_ == empty)
                return false;

            if (sieve_ == saturated)
                return true;
        }

        // Compare masked fingerprint to sieve, for all masks of screen.
        for (type segment{}; segment <= row; ++segment)
        {
            const auto mask = masks(row, segment);
            if (bit_and(fingerprint, mask) == bit_and(sieve_, mask))
                return true;
        }

        // Not empty or saturated, not aligned with screen (full if max).
        return false;
    }
}

TEMPLATE
constexpr bool CLASS::screen(type fingerprint) NOEXCEPT
{
    if constexpr (disabled)
    {
        return false;
    }
    else
    {
        using namespace system;
        auto row = shift_right(sieve_, screen_bits);
        if (row == limit)
        {
            if (sieve_ == empty)
            {
                // Reset empty sentinel (not screened) for first screen.
                zeroize(row);
            }
            else
            {
                // Sieve was full, now saturated (all screened).
                sieve_ = saturated;
                return false;
            }
        }
        else
        {
            if (screened(fingerprint))
            {
                // Screened, bucket may contain element.
                return false;
            }
            else
            {
                // Not screened, empty, or full - add screen.
                ++row;
            }
        }

        // Both indexes are screen selector, as each new screen adds one mask.
        const auto mask = masks(row, row);

        // Merge incremented selector, current sieve, and new fingerprint.
        sieve_ = bit_or
        (
            shift_left(row, screen_bits),
            bit_and
            (
                selector_mask,
                bit_or
                (
                    bit_and(fingerprint, mask),
                    bit_and(sieve_, bit_not(mask))
                )
            )
        );

        return true;
    }
}

// protected/static
TEMPLATE
CONSTEVAL CLASS::offsets_t CLASS::generate_offsets() NOEXCEPT
{
    using namespace system;

    // Generate compression offsets at compile, generally 16 or 32 elements.
    offsets_t offsets{};
    for (type index{}; index < screens; ++index)
        offsets[index] = to_half(ceilinged_multiply(index, add1(index)));

    return offsets;
}

// protected/static
TEMPLATE
CONSTEVAL CLASS::masks_t CLASS::generate_masks() NOEXCEPT
{
    using namespace system;
    masks_t out{};

    // Read/write compressed array as if it was a two-dimesional array.
    const auto masks = [&out](auto row, auto column) NOEXCEPT -> type&
    {
        BC_ASSERT(column <= row);
        constexpr auto get_offset = generate_offsets();
        return out[get_offset[row] + column];
    };

    // Determine the count of mask bits for a given table element.
    const auto mask_bits = [](auto row, auto column) NOEXCEPT -> size_t
    {
        BC_ASSERT(column <= row);
        const auto div = floored_divide(screen_bits, add1(row));
        const auto mod = floored_modulo(screen_bits, add1(row));
        return div + to_int(column < mod);
    };

    // Start with all screen bits set.
    masks(0, 0) = first_mask;

    // Progressively divide previous row masks into subsequent row.
    // Each row adds one mask, with previous row masks sacrificing bits for it.
    for (type row = 1; row < screens; ++row)
    {
        for (type col = 0; col < row; ++col)
            masks(row, col) = masks(sub1(row), col);

        for (auto mask = row; !is_zero(mask); --mask)
        {
            const auto col = sub1(mask);
            auto excess = floored_subtract(ones_count(masks(row, col)),
                mask_bits(row, col));

            while (!is_zero(excess))
            {
                const auto bit = right_zeros(masks(row, col));
                if (bit < screen_bits)
                {
                    set_right_into(masks(row, col), bit, false);
                    set_right_into(masks(row, row), bit, true);
                    --excess;
                }
            }
        }
    }

    // Unset last row high order screen bit to avoid sentinel conflict.
    // This may result in one empty mask, and therefore a false positive.
    for (type col = 0; col < screens; ++col)
        set_right_into(masks(sub1(screens), col), sentinel, false);

    return out;
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
