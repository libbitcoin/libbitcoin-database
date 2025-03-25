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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(sieve_tests)

using namespace system;

// 4 + 28 = 32 bit screens.
constexpr auto empty = 0xffffffff_u32;
constexpr auto saturated = 0xf8000000_u32;
constexpr auto selector_bits = sizeof(uint32_t);
constexpr auto screen_bits = bits<uint32_t> - selector_bits;
constexpr auto screens = power2(selector_bits);
constexpr auto selector_max = sub1(screens);
constexpr auto selector_mask = mask_left<uint32_t>(selector_bits);

static_assert(empty == 0b1111'1111111111111111111111111111);
static_assert(saturated == 0b1111'1000000000000000000000000000);
static_assert(selector_mask == 0b0000'1111111111111111111111111111);
static_assert(screen_bits == 28);
static_assert(selector_bits == 4);
static_assert(selector_max == 15);
static_assert(screens == 16);

constexpr uint32_t get_hash(const hash_digest& hash)
{
    uint32_t value{};
    from_little(value, hash);
    return value;
}

using screen_t = std_array<uint32_t, screens>;
using matrix_t = std_array<screen_t, screens>;
constexpr matrix_t matrix
{
    screen_t
    {
        // 1
        0b0000'1111111111111111111111111111
    },
    {
        // 2
        0b0000'1111111111111100000000000000,
        0b0000'0000000000000011111111111111
    },
    {
        // 3
        0b0000'1111111111000000000000000000,
        0b0000'0000000000000011111111100000,
        0b0000'0000000000111100000000011111
    },
    {
        // 4
        0b0000'1111111000000000000000000000,
        0b0000'0000000000000011111110000000,
        0b0000'0000000000111100000000011100,
        0b0000'0000000111000000000001100011
    },
    {
        // 5
        0b0000'1111110000000000000000000000,
        0b0000'0000000000000011111100000000,
        0b0000'0000000000111100000000011000,
        0b0000'0000000111000000000001100000,
        0b0000'0000001000000000000010000111
    },
    {
        // 6
        0b0000'1111100000000000000000000000,
        0b0000'0000000000000011111000000000,
        0b0000'0000000000111100000000010000,
        0b0000'0000000111000000000001100000,
        0b0000'0000001000000000000010000110,
        0b0000'0000010000000000000100001001
    },
    {
        // 7
        0b0000'1111000000000000000000000000,
        0b0000'0000000000000011110000000000,
        0b0000'0000000000111100000000000000,
        0b0000'0000000111000000000001000000,
        0b0000'0000001000000000000010000110,
        0b0000'0000010000000000000100001001,
        0b0000'0000100000000000001000110000
    },
    {
        // 8
        0b0000'1111000000000000000000000000,
        0b0000'0000000000000011110000000000,
        0b0000'0000000000111100000000000000,
        0b0000'0000000111000000000001000000,
        0b0000'0000001000000000000010000100,
        0b0000'0000010000000000000100001000,
        0b0000'0000100000000000001000100000,
        0b0000'0000000000000000000000010011
    },
    {
        // 9
        0b0000'1111000000000000000000000000,
        0b0000'0000000000000011100000000000,
        0b0000'0000000000111000000000000000,
        0b0000'0000000111000000000000000000,
        0b0000'0000001000000000000010000100,
        0b0000'0000010000000000000100001000,
        0b0000'0000100000000000001000100000,
        0b0000'0000000000000000000000010011,
        0b0000'0000000000000100010001000000
    },
    {
        // 10
        0b0000'1110000000000000000000000000,
        0b0000'0000000000000011100000000000,
        0b0000'0000000000111000000000000000,
        0b0000'0000000111000000000000000000,
        0b0000'0000001000000000000010000100,
        0b0000'0000010000000000000100001000,
        0b0000'0000100000000000001000100000,
        0b0000'0000000000000000000000010011,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000
    },
    {
        // 11
        0b0000'1110000000000000000000000000,
        0b0000'0000000000000011100000000000,
        0b0000'0000000000111000000000000000,
        0b0000'0000000111000000000000000000,
        0b0000'0000001000000000000010000100,
        0b0000'0000010000000000000100001000,
        0b0000'0000100000000000001000000000,
        0b0000'0000000000000000000000010010,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000,
        0b0000'0000000000000000000000100001
    },
    {
        // 12
        0b0000'1110000000000000000000000000,
        0b0000'0000000000000011100000000000,
        0b0000'0000000000111000000000000000,
        0b0000'0000000111000000000000000000,
        0b0000'0000001000000000000010000000,
        0b0000'0000010000000000000100000000,
        0b0000'0000100000000000001000000000,
        0b0000'0000000000000000000000010010,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000,
        0b0000'0000000000000000000000100001,
        0b0000'0000000000000000000000001100
    },
    {
        // 13
        0b0000'1110000000000000000000000000,
        0b0000'0000000000000011100000000000,
        0b0000'0000000000110000000000000000,
        0b0000'0000000110000000000000000000,
        0b0000'0000001000000000000010000000,
        0b0000'0000010000000000000100000000,
        0b0000'0000100000000000001000000000,
        0b0000'0000000000000000000000010010,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000,
        0b0000'0000000000000000000000100001,
        0b0000'0000000000000000000000001100,
        0b0000'0000000001001000000000000000
    },
    {
        // 14
        0b0000'1100000000000000000000000000,
        0b0000'0000000000000011000000000000,
        0b0000'0000000000110000000000000000,
        0b0000'0000000110000000000000000000,
        0b0000'0000001000000000000010000000,
        0b0000'0000010000000000000100000000,
        0b0000'0000100000000000001000000000,
        0b0000'0000000000000000000000010010,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000,
        0b0000'0000000000000000000000100001,
        0b0000'0000000000000000000000001100,
        0b0000'0000000001001000000000000000,
        0b0000'0010000000000000100000000000
    },
    {
        // 15
        0b0000'1100000000000000000000000000,
        0b0000'0000000000000011000000000000,
        0b0000'0000000000110000000000000000,
        0b0000'0000000110000000000000000000,
        0b0000'0000001000000000000010000000,
        0b0000'0000010000000000000100000000,
        0b0000'0000100000000000001000000000,
        0b0000'0000000000000000000000010010,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000,
        0b0000'0000000000000000000000100001,
        0b0000'0000000000000000000000001100,
        0b0000'0000000001001000000000000000,
        0b0000'0010000000000000000000000000,
        0b0000'0000000000000000100000000000
    },
    {
        // 16
        0b0000'0'100000000000000000000000000, // first bit sacrificed for sentinel
        0b0000'0000000000000011000000000000,
        0b0000'0000000000110000000000000000,
        0b0000'0000000110000000000000000000,
        0b0000'0000001000000000000010000000,
        0b0000'0000010000000000000100000000,
        0b0000'0000100000000000001000000000,
        0b0000'0000000000000000000000010010,
        0b0000'0000000000000100010000000000,
        0b0000'0001000000000000000001000000,
        0b0000'0000000000000000000000100001,
        0b0000'0000000000000000000000001100,
        0b0000'0000000001000000000000000000,
        0b0000'0010000000000000000000000000,
        0b0000'0000000000000000100000000000,
        0b0000'0000000000001000000000000000
    }
};

constexpr std_array<hash_digest, 18> vectors
{
    base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"),
    base16_hash("0e3e2357e806b6cdb1f70b54c3a3a17b6714ee1f0e68bebb44a74b1efd512098"),
    base16_hash("9b0fc922603d9202f66d6026d83505e668e7444d9480d3a1b28e4670d548e092"),
    base16_hash("f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16"), // a (same tx)
    base16_hash("d1b7f1d38eaae671d897aff7e7cf7b69c8afaf773bcbf8f5f4f2d9e8e9f6e8e2"),
    base16_hash("6f7c9d6b8e1d5e7f8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e"),
    base16_hash("a16f3ce4dd5deb92d90424cb7d9e7f875e257450a7d4b8d13fc39b3a25b3c3ab"),
    base16_hash("b1fea52486ce0c62bb442b530a3f0132b826c74e473d1f2c220bfa78111c5082"),
    base16_hash("f5d8ee39a430901c91a5917b9f2dc19d6d1a0e9cea205b009ca73dd04470b9a6"),
    base16_hash("d5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599"),
    base16_hash("e3706273a032e456c1f8f6a2f9053b90d5f9ab33f8548b6c851627a025b3f8df"),
    base16_hash("6bced924b3e90f9f738e4d320b2f900e8f6b6f6b6f6b6f6b6f6b6f6b6f6b6f6b"), // b (similar tx)
    base16_hash("c7f2d3e3b9e7f8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5"), // c (similar tx)
    base16_hash("e9a668c1b9e7f8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5e8b1e8f5"), // c (similar tx)
    base16_hash("b3e90f9f738e4d320b2f900e8f6b6f6b6f6b6f6b6f6b6f6b6f6b6f6b6f6b6f6b"), // b (similar tx)
    base16_hash("f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16"), // a (same tx)
    base16_hash("1111111111111111111111111111111111111111111111111111111111111111"),
    base16_hash("0000000000000000000000000000000000000000000000000000000000000000")
};

constexpr std_array<uint32_t, 18> values
{
    get_hash(vectors[0x00]),
    get_hash(vectors[0x01]),
    get_hash(vectors[0x02]),
    get_hash(vectors[0x03]),
    get_hash(vectors[0x04]),
    get_hash(vectors[0x05]),
    get_hash(vectors[0x06]),
    get_hash(vectors[0x07]),
    get_hash(vectors[0x08]),
    get_hash(vectors[0x09]),
    get_hash(vectors[0x0a]),
    get_hash(vectors[0x0b]),
    get_hash(vectors[0x0c]),
    get_hash(vectors[0x0d]),
    get_hash(vectors[0x0e]),
    get_hash(vectors[0x0f]),
    get_hash(vectors[0x10]),
    get_hash(vectors[0x11])
};

BOOST_AUTO_TEST_CASE(sieve__vector__values__expected)
{
    BOOST_REQUIRE_EQUAL(values[0x00], 4260209467u);
    BOOST_REQUIRE_EQUAL(values[0x01], 4249952408u);
    BOOST_REQUIRE_EQUAL(values[0x02], 3578323090u);
    BOOST_REQUIRE_EQUAL(values[0x03], 2199821846u); // a (same tx)
    BOOST_REQUIRE_EQUAL(values[0x04], 3925272802u);
    BOOST_REQUIRE_EQUAL(values[0x05], 2405337886u);
    BOOST_REQUIRE_EQUAL(values[0x06],  632538027u);
    BOOST_REQUIRE_EQUAL(values[0x07],  287068290u);
    BOOST_REQUIRE_EQUAL(values[0x08], 1148238246u);
    BOOST_REQUIRE_EQUAL(values[0x09],  702055833u);
    BOOST_REQUIRE_EQUAL(values[0x0a],  632551647u);
    BOOST_REQUIRE_EQUAL(values[0x0b], 1869311851u); // b (similar tx)
    BOOST_REQUIRE_EQUAL(values[0x0c], 3903973621u); // c (similar tx)
    BOOST_REQUIRE_EQUAL(values[0x0d], 3903973621u); // c (similar tx)
    BOOST_REQUIRE_EQUAL(values[0x0e], 1869311851u); // b (similar tx)
    BOOST_REQUIRE_EQUAL(values[0x0f], 2199821846u); // a (same tx)
    BOOST_REQUIRE_EQUAL(values[0x10], 286331153u);
    BOOST_REQUIRE_EQUAL(values[0x11], 0u);
}

BOOST_AUTO_TEST_CASE(sieve__matrix__non_zero_count__expected)
{
    const auto non_zeros = [](size_t index)
    {
        return std::accumulate(matrix[index].begin(), matrix[index].end(), zero,
            [](size_t total, uint32_t value)
            {
                return total + to_int(to_bool(value));
            });
    };

    // Number of relevant masks by row (positions not assured).
    BOOST_REQUIRE_EQUAL(non_zeros(0x0), 1u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x1), 2u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x2), 3u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x3), 4u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x4), 5u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x5), 6u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x6), 7u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x7), 8u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x8), 9u);
    BOOST_REQUIRE_EQUAL(non_zeros(0x9), 10u);
    BOOST_REQUIRE_EQUAL(non_zeros(0xa), 11u);
    BOOST_REQUIRE_EQUAL(non_zeros(0xb), 12u);
    BOOST_REQUIRE_EQUAL(non_zeros(0xc), 13u);
    BOOST_REQUIRE_EQUAL(non_zeros(0xd), 14u);
    BOOST_REQUIRE_EQUAL(non_zeros(0xe), 15u);
    BOOST_REQUIRE_EQUAL(non_zeros(0xf), 16u);
}

BOOST_AUTO_TEST_CASE(sieve__matrix__evens__expected)
{
    constexpr auto screen_bits = bits<uint32_t> - sizeof(uint32_t);

    const auto evens = [](size_t index)
    {
        const auto even = floored_divide(screen_bits, add1(index));
        return std::accumulate(matrix[index].begin(), matrix[index].end(), zero,
            [&](size_t total, uint32_t value)
            {
                return total + to_int(ones_count(value) == even);
            });
    };

    const auto expected_evens = [](size_t index)
    {
        const auto odds = floored_modulo(screen_bits, add1(index));
        return add1(index) - odds;
    };

    // Number of non-rounded masks by row (positions not assured).
    BOOST_REQUIRE_EQUAL(evens(0x0), expected_evens(0x0));
    BOOST_REQUIRE_EQUAL(evens(0x1), expected_evens(0x1));
    BOOST_REQUIRE_EQUAL(evens(0x2), expected_evens(0x2));
    BOOST_REQUIRE_EQUAL(evens(0x3), expected_evens(0x3));
    BOOST_REQUIRE_EQUAL(evens(0x4), expected_evens(0x4));
    BOOST_REQUIRE_EQUAL(evens(0x5), expected_evens(0x5));
    BOOST_REQUIRE_EQUAL(evens(0x6), expected_evens(0x6));
    BOOST_REQUIRE_EQUAL(evens(0x7), expected_evens(0x7));
    BOOST_REQUIRE_EQUAL(evens(0x8), expected_evens(0x8));
    BOOST_REQUIRE_EQUAL(evens(0x9), expected_evens(0x9));
    BOOST_REQUIRE_EQUAL(evens(0xa), expected_evens(0xa));
    BOOST_REQUIRE_EQUAL(evens(0xb), expected_evens(0xb));
    BOOST_REQUIRE_EQUAL(evens(0xc), expected_evens(0xc));
    BOOST_REQUIRE_EQUAL(evens(0xd), expected_evens(0xd));
    BOOST_REQUIRE_EQUAL(evens(0xe), expected_evens(0xe));
    BOOST_REQUIRE_EQUAL(evens(0xf), add1(expected_evens(0xf)));
}

BOOST_AUTO_TEST_CASE(sieve__matrix__odds__expected)
{
    constexpr auto screen_bits = bits<uint32_t> -sizeof(uint32_t);

    const auto odds = [](size_t index)
    {
        const auto odd = add1(floored_divide(screen_bits, add1(index)));
        return std::accumulate(matrix[index].begin(), matrix[index].end(), zero,
            [&](size_t total, uint32_t value)
            {
                return total + to_bool(ones_count(value) == odd);
            });
    };

    const auto expected_odds = [](size_t index)
    {
        return floored_modulo(screen_bits, add1(index));
    };

    // Number of rounded up masks by row (positions not assured).
    BOOST_REQUIRE_EQUAL(odds(0x0), expected_odds(0x0));
    BOOST_REQUIRE_EQUAL(odds(0x1), expected_odds(0x1));
    BOOST_REQUIRE_EQUAL(odds(0x2), expected_odds(0x2));
    BOOST_REQUIRE_EQUAL(odds(0x3), expected_odds(0x3));
    BOOST_REQUIRE_EQUAL(odds(0x4), expected_odds(0x4));
    BOOST_REQUIRE_EQUAL(odds(0x5), expected_odds(0x5));
    BOOST_REQUIRE_EQUAL(odds(0x6), expected_odds(0x6));
    BOOST_REQUIRE_EQUAL(odds(0x7), expected_odds(0x7));
    BOOST_REQUIRE_EQUAL(odds(0x8), expected_odds(0x8));
    BOOST_REQUIRE_EQUAL(odds(0x9), expected_odds(0x9));
    BOOST_REQUIRE_EQUAL(odds(0xa), expected_odds(0xa));
    BOOST_REQUIRE_EQUAL(odds(0xb), expected_odds(0xb));
    BOOST_REQUIRE_EQUAL(odds(0xc), expected_odds(0xc));
    BOOST_REQUIRE_EQUAL(odds(0xd), expected_odds(0xd));
    BOOST_REQUIRE_EQUAL(odds(0xe), expected_odds(0xe));
    BOOST_REQUIRE_EQUAL(odds(0xf), sub1(expected_odds(0xf)));
}

BOOST_AUTO_TEST_CASE(sieve__matrix__accumulation__expected)
{
    constexpr auto expected = sub1(power2(screen_bits));
    const auto combiner = [](size_t index)
    {
        return std::accumulate(matrix[index].begin(), matrix[index].end(), 0u,
            [](uint32_t total, uint32_t value)
            {
                return bit_xor(total, value);
            });
    };

    // One bit per column (summation not strictly assured).
    BOOST_REQUIRE_EQUAL(combiner(0x0), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x1), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x2), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x3), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x4), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x5), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x6), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x7), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x8), expected);
    BOOST_REQUIRE_EQUAL(combiner(0x9), expected);
    BOOST_REQUIRE_EQUAL(combiner(0xa), expected);
    BOOST_REQUIRE_EQUAL(combiner(0xb), expected);
    BOOST_REQUIRE_EQUAL(combiner(0xc), expected);
    BOOST_REQUIRE_EQUAL(combiner(0xd), expected);
    BOOST_REQUIRE_EQUAL(combiner(0xe), expected);
    BOOST_REQUIRE_EQUAL(combiner(0xf), set_right(expected, sub1(screen_bits), false));
}

// Sieve is unused, nothing is screened.
constexpr bool is_empty(uint32_t sieve)
{
    return sieve == empty;
}

// Sieve is saturated, everything is presumed screened.
constexpr bool is_saturated(uint32_t sieve)
{
    return sieve == saturated;
}

// Selector indicates that sieve may be empty or saturated.
constexpr bool is_maximum(size_t selector)
{
    return selector == selector_max;
}

// Selector is the zero-based index of the current screen.
constexpr uint32_t get_selector(uint32_t sieve)
{
    return shift_right(sieve, screen_bits);
}

// True if fingerprint aligns with sieve screen(s), bucket may contain element.
constexpr bool is_screened(uint32_t sieve, uint32_t print)
{
    const auto selector = get_selector(sieve);

    if (is_maximum(selector))
    {
        if (is_empty(sieve))
            return false;

        if (is_saturated(sieve))
            return true;
    }

    // Compare masked fingerprint to masked sieve, for all masks of the screen.
    for (auto mask = zero; mask <= selector; ++mask)
        if (bit_and(print, matrix[selector][mask]) ==
            bit_and(sieve, matrix[selector][mask]))
            return true;

    // Not empty, not saturated, not aligned with active screen (full if max).
    return false;
}

constexpr bool add_finger(uint32_t& sieve, uint32_t print)
{
    // Saturated or aligned, bucket may contain element.
    if (is_screened(sieve, print))
        return false;

    // Empty or unsaturated, bucket does not contain element.
    auto selector = get_selector(sieve);
    if (is_maximum(selector))
    {
        if (is_empty(sieve))
        {
            // Reset empty sentinel for first element and continue.
            zeroize(selector);
        }
        else
        {
            // Sieve was full, set to saturated and return.
            sieve = saturated;
            return false;
        }
    }
    else
    {
        ++selector;
    }

    // Both indexes are screen selector, as each new screen adds one mask.
    const auto mask = matrix[selector][selector];

    // Merge incremented selector, current sieve, and new fingerprint.
    sieve = bit_or
    (
        shift_left(selector, screen_bits),
        bit_and
        (
            selector_mask,
            bit_or
            (
                bit_and(print, mask),
                bit_and(sieve, bit_not(mask))
            )
        )
    );

    return true;
}

// test helper.
constexpr uint32_t add_finger_const(uint32_t sieve, uint32_t fingerprint) NOEXCEPT
{
    // empty after insert implies false result (screened, not added).
    return add_finger(sieve, fingerprint) ? sieve : empty;
}

constexpr auto sieve0 = empty;
static_assert(is_empty(sieve0));
static_assert(!is_saturated(sieve0));
static_assert(!is_screened(sieve0, values[0]));
static_assert(get_selector(sieve0) == selector_max);

constexpr auto sieve1 = add_finger_const(sieve0, values[0]);
static_assert(!is_empty(sieve1));
static_assert(!is_saturated(sieve1));
static_assert(is_screened(sieve1, values[0]));
static_assert(!is_screened(sieve1, values[1]));
static_assert(get_selector(sieve1) == 0u);

constexpr auto sieve2 = add_finger_const(sieve1, values[1]);
static_assert(!is_empty(sieve2));
static_assert(!is_saturated(sieve2));
static_assert(is_screened(sieve2, values[0]));
static_assert(is_screened(sieve2, values[1]));
static_assert(!is_screened(sieve2, values[2]));
static_assert(get_selector(sieve2) == 1u);

constexpr auto sieve3 = add_finger_const(sieve2, values[2]);
static_assert(!is_empty(sieve3));
static_assert(!is_saturated(sieve3));
static_assert(is_screened(sieve3, values[0]));
static_assert(is_screened(sieve3, values[1]));
static_assert(is_screened(sieve3, values[2]));
static_assert(!is_screened(sieve3, values[3]));
static_assert(get_selector(sieve3) == 2u);

constexpr auto sieve4 = add_finger_const(sieve3, values[3]);
static_assert(!is_empty(sieve4));
static_assert(!is_saturated(sieve4));
static_assert(is_screened(sieve4, values[0]));
static_assert(is_screened(sieve4, values[1]));
static_assert(is_screened(sieve4, values[2]));
static_assert(is_screened(sieve4, values[3]));
static_assert(!is_screened(sieve4, values[4]));
static_assert(get_selector(sieve4) == 3u);

constexpr auto sieve5 = add_finger_const(sieve4, values[4]);
static_assert(!is_empty(sieve5));
static_assert(is_screened(sieve5, values[0]));
static_assert(is_screened(sieve5, values[1]));
static_assert(is_screened(sieve5, values[2]));
static_assert(is_screened(sieve5, values[3]));
static_assert(is_screened(sieve5, values[4]));
static_assert(is_screened(sieve5, values[5])); // <==
static_assert(get_selector(sieve5) == 4u);

// values[5] is already screened, so not added.
constexpr auto sieve6 = add_finger_const(sieve5, values[5]);
static_assert(is_empty(sieve6));
static_assert(is_screened(sieve5, values[5]));
static_assert(!is_screened(sieve5, values[6]));

constexpr auto sieve7 = add_finger_const(sieve5, values[6]);
static_assert(!is_empty(sieve7));
static_assert(is_screened(sieve7, values[0]));
static_assert(is_screened(sieve7, values[1]));
static_assert(is_screened(sieve7, values[2]));
static_assert(is_screened(sieve7, values[3]));
static_assert(is_screened(sieve7, values[4]));
static_assert(is_screened(sieve7, values[5]));
static_assert(is_screened(sieve7, values[6]));
static_assert(is_screened(sieve7, values[7])); // <==
static_assert(get_selector(sieve7) == 5u);

// values[7] is already screened, so not added.
constexpr auto sieve8 = add_finger_const(sieve7, values[7]);
static_assert(is_empty(sieve8));
static_assert(is_screened(sieve7, values[7]));
static_assert(!is_screened(sieve7, values[8]));

constexpr auto sieve9 = add_finger_const(sieve7, values[8]);
static_assert(!is_empty(sieve9));
static_assert(is_screened(sieve9, values[0]));
static_assert(is_screened(sieve9, values[1]));
static_assert(is_screened(sieve9, values[2]));
static_assert(is_screened(sieve9, values[3]));
static_assert(is_screened(sieve9, values[4]));
static_assert(is_screened(sieve9, values[5]));
static_assert(is_screened(sieve9, values[6]));
static_assert(is_screened(sieve9, values[7]));
static_assert(is_screened(sieve9, values[8]));
static_assert(!is_screened(sieve9, values[9]));
static_assert(get_selector(sieve9) == 6u);

constexpr auto sieve10 = add_finger_const(sieve9, values[9]);
static_assert(!is_empty(sieve10));
static_assert(is_screened(sieve10, values[0]));
static_assert(is_screened(sieve10, values[1]));
static_assert(is_screened(sieve10, values[2]));
static_assert(is_screened(sieve10, values[3]));
static_assert(is_screened(sieve10, values[4]));
static_assert(is_screened(sieve10, values[5]));
static_assert(is_screened(sieve10, values[6]));
static_assert(is_screened(sieve10, values[7]));
static_assert(is_screened(sieve10, values[8]));
static_assert(is_screened(sieve10, values[9]));
static_assert(!is_screened(sieve10, values[10]));
static_assert(get_selector(sieve10) == 7u);

constexpr auto sieve11 = add_finger_const(sieve10, values[10]);
static_assert(!is_empty(sieve11));
static_assert(is_screened(sieve11, values[0]));
static_assert(is_screened(sieve11, values[1]));
static_assert(is_screened(sieve11, values[2]));
static_assert(is_screened(sieve11, values[3]));
static_assert(is_screened(sieve11, values[4]));
static_assert(is_screened(sieve11, values[5]));
static_assert(is_screened(sieve11, values[6]));
static_assert(is_screened(sieve11, values[7]));
static_assert(is_screened(sieve11, values[8]));
static_assert(is_screened(sieve11, values[9]));
static_assert(is_screened(sieve11, values[10]));
static_assert(!is_screened(sieve11, values[11]));
static_assert(get_selector(sieve11) == 8u);

constexpr auto sieve12 = add_finger_const(sieve11, values[11]);
static_assert(!is_empty(sieve12));
static_assert(is_screened(sieve12, values[0]));
static_assert(is_screened(sieve12, values[1]));
static_assert(is_screened(sieve12, values[2]));
static_assert(is_screened(sieve12, values[3]));
static_assert(is_screened(sieve12, values[4]));
static_assert(is_screened(sieve12, values[5]));
static_assert(is_screened(sieve12, values[6]));
static_assert(is_screened(sieve12, values[7]));
static_assert(is_screened(sieve12, values[8]));
static_assert(is_screened(sieve12, values[9]));
static_assert(is_screened(sieve12, values[10]));
static_assert(is_screened(sieve12, values[11]));
static_assert(is_screened(sieve12, values[12])); // <==
static_assert(get_selector(sieve12) == 9u);

// values[12] is already screened, so not added.
constexpr auto sieve13 = add_finger_const(sieve12, values[12]);
static_assert(is_empty(sieve13));
static_assert(is_screened(sieve12, values[12]));
static_assert(is_screened(sieve12, values[13])); // <==

// values[13] is already screened, so not added.
constexpr auto sieve14 = add_finger_const(sieve12, values[13]);
static_assert(is_empty(sieve14));
static_assert(is_screened(sieve12, values[13]));
static_assert(is_screened(sieve12, values[14])); // <==

// values[14] is already screened, so not added.
constexpr auto sieve15 = add_finger_const(sieve12, values[14]);
static_assert(is_empty(sieve15));
static_assert(is_screened(sieve12, values[14]));
static_assert(is_screened(sieve12, values[15])); // <==

// values[15] is already screened, so not added.
constexpr auto sieve16 = add_finger_const(sieve12, values[15]);
static_assert(is_empty(sieve16));
static_assert(is_screened(sieve12, values[15]));
static_assert(is_screened(sieve12, values[16])); // <==

// values[16] is already screened, so not added.
constexpr auto sieve17 = add_finger_const(sieve12, values[16]);
static_assert(is_empty(sieve17));
static_assert(is_screened(sieve12, values[16]));
static_assert(!is_screened(sieve12, values[17]));

constexpr auto sieve18 = add_finger_const(sieve12, values[17]);
static_assert(!is_empty(sieve18));
static_assert(is_screened(sieve18, values[0]));
static_assert(is_screened(sieve18, values[1]));
static_assert(is_screened(sieve18, values[2]));
static_assert(is_screened(sieve18, values[3]));
static_assert(is_screened(sieve18, values[4]));
static_assert(is_screened(sieve18, values[5]));
static_assert(is_screened(sieve18, values[6]));
static_assert(is_screened(sieve18, values[7]));
static_assert(is_screened(sieve18, values[8]));
static_assert(is_screened(sieve18, values[9]));
static_assert(is_screened(sieve18, values[10]));
static_assert(is_screened(sieve18, values[11]));
static_assert(is_screened(sieve18, values[12]));
static_assert(is_screened(sieve18, values[13]));
static_assert(is_screened(sieve18, values[14]));
static_assert(is_screened(sieve18, values[15]));
static_assert(is_screened(sieve18, values[16]));
static_assert(is_screened(sieve18, values[17]));
static_assert(get_selector(sieve18) == 10u);

BOOST_AUTO_TEST_CASE(sieve_test)
{
    constexpr auto length = bits<uint32_t>;

    auto sieve = empty;
    std::cout << "sieve [-] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(!is_screened(sieve, values[0]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), selector_max);

    BOOST_REQUIRE(add_finger(sieve, values[0]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [0] : " << binary(length, to_big(values[0])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[0][0])) << std::endl;
    std::cout << "sieve [0] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 0u);

    BOOST_REQUIRE(add_finger(sieve, values[1]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [1] : " << binary(length, to_big(values[1])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[1][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[1][1])) << std::endl;
    std::cout << "sieve [1] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 1u);

    BOOST_REQUIRE(add_finger(sieve, values[2]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [2] : " << binary(length, to_big(values[2])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[2][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[2][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[2][2])) << std::endl;
    std::cout << "sieve [2] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 2u);

    BOOST_REQUIRE(add_finger(sieve, values[3]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [3] : " << binary(length, to_big(values[3])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[3][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[3][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[3][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[3][3])) << std::endl;
    std::cout << "sieve [3] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 3u);

    BOOST_REQUIRE(add_finger(sieve, values[4]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [4] : " << binary(length, to_big(values[4])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[4][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[4][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[4][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[4][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[4][4])) << std::endl;
    std::cout << "sieve [4] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 4u);

    // values[5] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[5]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [5] : " << binary(length, to_big(values[5])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[4][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[4][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[4][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[4][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[4][4])) << std::endl;
    std::cout << "sieve [4] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 4u);

    BOOST_REQUIRE(add_finger(sieve, values[6]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [6] : " << binary(length, to_big(values[6])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[5][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[5][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[5][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[5][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[5][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[5][5])) << std::endl;
    std::cout << "sieve [5] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 5u);

    // values[7] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[7]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [7] : " << binary(length, to_big(values[7])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[5][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[5][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[5][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[5][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[5][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[5][5])) << std::endl;
    std::cout << "sieve [5] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 5u);

    BOOST_REQUIRE(add_finger(sieve, values[8]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [8] : " << binary(length, to_big(values[8])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[6][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[6][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[6][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[6][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[6][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[6][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[6][6])) << std::endl;
    std::cout << "sieve [6] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 6u);

    BOOST_REQUIRE(add_finger(sieve, values[9]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [9] : " << binary(length, to_big(values[9])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[7][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[7][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[7][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[7][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[7][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[7][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[7][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[7][7])) << std::endl;
    std::cout << "sieve [7] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 7u);

    BOOST_REQUIRE(add_finger(sieve, values[10]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [10]: " << binary(length, to_big(values[10])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[8][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[8][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[8][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[8][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[8][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[8][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[8][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[8][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[8][8])) << std::endl;
    std::cout << "sieve [8] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 8u);

    BOOST_REQUIRE(add_finger(sieve, values[11]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [11]: " << binary(length, to_big(values[11])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[9][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[9][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[9][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[9][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[9][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[9][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[9][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[9][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[9][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[9][9])) << std::endl;
    std::cout << "sieve [9] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 9u);

    // values[12] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[12]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [12]: " << binary(length, to_big(values[12])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[9][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[9][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[9][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[9][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[9][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[9][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[9][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[9][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[9][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[9][9])) << std::endl;
    std::cout << "sieve [9] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 9u);

    // values[13] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[13]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [13]: " << binary(length, to_big(values[13])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[9][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[9][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[9][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[9][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[9][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[9][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[9][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[9][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[9][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[9][9])) << std::endl;
    std::cout << "sieve [9] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 9u);

    // values[14] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[14]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [14]: " << binary(length, to_big(values[14])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[9][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[9][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[9][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[9][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[9][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[9][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[9][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[9][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[9][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[9][9])) << std::endl;
    std::cout << "sieve [9] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 9u);

    // values[15] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[15]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [15]: " << binary(length, to_big(values[15])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[9][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[9][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[9][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[9][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[9][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[9][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[9][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[9][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[9][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[9][9])) << std::endl;
    std::cout << "sieve [9] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 9u);

    // values[16] is already screened, so not added.
    BOOST_REQUIRE(!add_finger(sieve, values[16]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [16]: " << binary(length, to_big(values[16])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[9][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[9][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[9][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[9][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[9][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[9][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[9][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[9][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[9][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[9][9])) << std::endl;
    std::cout << "sieve [9] : " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 9u);

    BOOST_REQUIRE(add_finger(sieve, values[17]));
    std::cout << "-----------" << std::endl;
    std::cout << "value [17]: " << binary(length, to_big(values[17])) << std::endl;
    std::cout << "masks [0] : " << binary(length, to_big(matrix[10][0])) << std::endl;
    std::cout << "masks [1] : " << binary(length, to_big(matrix[10][1])) << std::endl;
    std::cout << "masks [2] : " << binary(length, to_big(matrix[10][2])) << std::endl;
    std::cout << "masks [3] : " << binary(length, to_big(matrix[10][3])) << std::endl;
    std::cout << "masks [4] : " << binary(length, to_big(matrix[10][4])) << std::endl;
    std::cout << "masks [5] : " << binary(length, to_big(matrix[10][5])) << std::endl;
    std::cout << "masks [6] : " << binary(length, to_big(matrix[10][6])) << std::endl;
    std::cout << "masks [7] : " << binary(length, to_big(matrix[10][7])) << std::endl;
    std::cout << "masks [8] : " << binary(length, to_big(matrix[10][8])) << std::endl;
    std::cout << "masks [9] : " << binary(length, to_big(matrix[10][9])) << std::endl;
    std::cout << "masks [10]: " << binary(length, to_big(matrix[10][10])) << std::endl;
    std::cout << "sieve [10]: " << binary(length, to_big(sieve)) << std::endl;

    BOOST_REQUIRE(!is_empty(sieve));
    BOOST_REQUIRE(!is_saturated(sieve));
    BOOST_REQUIRE(is_screened(sieve, values[0]));
    BOOST_REQUIRE(is_screened(sieve, values[1]));
    BOOST_REQUIRE(is_screened(sieve, values[2]));
    BOOST_REQUIRE(is_screened(sieve, values[3]));
    BOOST_REQUIRE(is_screened(sieve, values[4]));
    BOOST_REQUIRE(is_screened(sieve, values[5]));
    BOOST_REQUIRE(is_screened(sieve, values[6]));
    BOOST_REQUIRE(is_screened(sieve, values[7]));
    BOOST_REQUIRE(is_screened(sieve, values[8]));
    BOOST_REQUIRE(is_screened(sieve, values[9]));
    BOOST_REQUIRE(is_screened(sieve, values[10]));
    BOOST_REQUIRE_EQUAL(get_selector(sieve), 10u);
}

BOOST_AUTO_TEST_SUITE_END()
