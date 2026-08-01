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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(memory_utilities_tests)

// It is presumed impossible for the page size to be zero, in which case this
// test should never fail on any platform, though an API failure returns zero.
BOOST_AUTO_TEST_CASE(memory_utilities__page_size__always__non_zero)
{
    BOOST_REQUIRE(is_nonzero(page_size()));
}

// It is not possible for the actual memory to be zero and an overflow will
// return max_uint64, so this test should never fail on any platform, though
// an API failure returns zero.
BOOST_AUTO_TEST_CASE(memory_utilities__system_memory__always__nonzero)
{
    BOOST_REQUIRE(is_nonzero(system_memory()));
}

// release_candidates

BOOST_AUTO_TEST_CASE(memory_utilities__release_candidates__none__all)
{
    BOOST_REQUIRE_EQUAL(release_candidates(0, 0, 0), max_uint64);
}

BOOST_AUTO_TEST_CASE(memory_utilities__release_candidates__each__excluded)
{
    BOOST_REQUIRE_EQUAL(release_candidates(0b0001, 0b0010, 0b0100),
        bit_not<uint64_t>(0b0111));
}

BOOST_AUTO_TEST_CASE(memory_utilities__release_candidates__all__none)
{
    BOOST_REQUIRE_EQUAL(release_candidates(max_uint64, 0, 0), zero);
    BOOST_REQUIRE_EQUAL(release_candidates(0, max_uint64, 0), zero);
    BOOST_REQUIRE_EQUAL(release_candidates(0, 0, max_uint64), zero);
}

// release_below
// The boundary word masking defect class: candidacy must retain LOW bits
// (pages below the count), never the high complement.

BOOST_AUTO_TEST_CASE(memory_utilities__release_below__word_above_count__zero)
{
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 64, 64), zero);
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 10, 64), zero);
}

BOOST_AUTO_TEST_CASE(memory_utilities__release_below__word_below_count__all)
{
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 64, 0), max_uint64);
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 128, 64), max_uint64);
}

BOOST_AUTO_TEST_CASE(memory_utilities__release_below__boundary__low_bits)
{
    // One full page: only bit zero (the historical inversion released 1..63).
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 1, 0), 0b0001u);
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 3, 0), 0b0111u);

    // Sixty-five pages: boundary word (first=64) retains only bit zero.
    BOOST_REQUIRE_EQUAL(release_below(max_uint64, 65, 64), 0b0001u);
}

// page_mask

BOOST_AUTO_TEST_CASE(memory_utilities__page_mask__disjoint__zero)
{
    BOOST_REQUIRE_EQUAL(page_mask(0, 10, 64), zero);
    BOOST_REQUIRE_EQUAL(page_mask(128, 130, 64), zero);
    BOOST_REQUIRE_EQUAL(page_mask(10, 10, 0), zero);
}

BOOST_AUTO_TEST_CASE(memory_utilities__page_mask__contained__expected)
{
    BOOST_REQUIRE_EQUAL(page_mask(0, 1, 0), 0b0001u);
    BOOST_REQUIRE_EQUAL(page_mask(1, 3, 0), 0b0110u);
    BOOST_REQUIRE_EQUAL(page_mask(65, 67, 64), 0b0110u);
}

BOOST_AUTO_TEST_CASE(memory_utilities__page_mask__spanning__clamped)
{
    // Run [60, 70) in word zero: bits 60..63; in word one: bits 0..5.
    BOOST_REQUIRE_EQUAL(page_mask(60, 70, 0),
        bit_and(max_uint64, mask_right<uint64_t>(60)));
    BOOST_REQUIRE_EQUAL(page_mask(60, 70, 64), unmask_right<uint64_t>(6));
}

BOOST_AUTO_TEST_CASE(memory_utilities__page_mask__full_word__all)
{
    BOOST_REQUIRE_EQUAL(page_mask(0, 128, 64), max_uint64);
}

// next_run

BOOST_AUTO_TEST_CASE(memory_utilities__next_run__empty__none)
{
    const uint64_t words[]{ 0, 0 };
    const auto run = next_run(words, 128, 0);
    BOOST_REQUIRE_EQUAL(run.first, run.second);
}

BOOST_AUTO_TEST_CASE(memory_utilities__next_run__single_page__found)
{
    const uint64_t words[]{ 0b01000, 0 };
    const auto run = next_run(words, 128, 0);
    BOOST_REQUIRE_EQUAL(run.first, 3u);
    BOOST_REQUIRE_EQUAL(run.second, 4u);
}

BOOST_AUTO_TEST_CASE(memory_utilities__next_run__word_spanning__joined)
{
    // Bits 62..63 of word zero and 0..2 of word one: run [62, 67).
    const uint64_t words[]{ mask_right<uint64_t>(62), 0b0111 };
    const auto run = next_run(words, 128, 0);
    BOOST_REQUIRE_EQUAL(run.first, 62u);
    BOOST_REQUIRE_EQUAL(run.second, 67u);
}

BOOST_AUTO_TEST_CASE(memory_utilities__next_run__from_skips__second)
{
    const uint64_t words[]{ 0b0011, 0b0011 };
    const auto run = next_run(words, 128, 2);
    BOOST_REQUIRE_EQUAL(run.first, 64u);
    BOOST_REQUIRE_EQUAL(run.second, 66u);
}

BOOST_AUTO_TEST_CASE(memory_utilities__next_run__page_bound__clipped)
{
    // All set but only 70 pages exist: run [0, 70).
    const uint64_t words[]{ max_uint64, max_uint64 };
    const auto run = next_run(words, 70, 0);
    BOOST_REQUIRE_EQUAL(run.first, zero);
    BOOST_REQUIRE_EQUAL(run.second, 70u);
}

// bit_run

BOOST_AUTO_TEST_CASE(memory_utilities__bit_run__unset_page__empty)
{
    const uint64_t words[]{ 0b0110, 0 };
    const auto run = bit_run(words, 128, 3);
    BOOST_REQUIRE_EQUAL(run.first, run.second);
}

BOOST_AUTO_TEST_CASE(memory_utilities__bit_run__interior_page__extent)
{
    const uint64_t words[]{ 0b111100, 0 };
    const auto run = bit_run(words, 128, 3);
    BOOST_REQUIRE_EQUAL(run.first, 2u);
    BOOST_REQUIRE_EQUAL(run.second, 6u);
}

BOOST_AUTO_TEST_CASE(memory_utilities__bit_run__word_spanning__extent)
{
    // Bits 63 of word zero and 0..1 of word one: run [63, 66) from page 64.
    const uint64_t words[]{ bit_left<uint64_t>(0), 0b0011 };
    const auto run = bit_run(words, 128, 64);
    BOOST_REQUIRE_EQUAL(run.first, 63u);
    BOOST_REQUIRE_EQUAL(run.second, 66u);
}

BOOST_AUTO_TEST_CASE(memory_utilities__bit_run__beyond_pages__empty)
{
    const uint64_t words[]{ max_uint64, max_uint64 };
    const auto run = bit_run(words, 64, 100);
    BOOST_REQUIRE_EQUAL(run.first, run.second);
}

BOOST_AUTO_TEST_SUITE_END()
