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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(history_tests)

using namespace system;

// less_than.operator()

BOOST_AUTO_TEST_CASE(history__less_than__confirmed_before_unconfirmed__expected)
{
    const history a{ { hash_digest{}, 42 }, 0, 7 };
    const history b{ { hash_digest{}, 0  }, 0, history::unconfirmed_position };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(history__less_than__confirmed_height_ascending__expected)
{
    const history a{ { hash_digest{}, 100 }, 0, 5 };
    const history b{ { hash_digest{}, 200 }, 0, 5 };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(history__less_than__confirmed_position_ascending__expected)
{
    const history a{ { hash_digest{}, 100 }, 0, 3 };
    const history b{ { hash_digest{}, 100 }, 0, 10 };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(history__less_than__unconfirmed_rooted_before_unrooted__expected)
{
    constexpr hash_digest hash1{ 0x10 };
    constexpr hash_digest hash2{ 0x01 };
    const history a{ { hash1, history::rooted_height }, 0, history::unconfirmed_position };
    const history b{ { hash2, history::unrooted_height }, 0, history::unconfirmed_position };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(history__less_than__unconfirmed_hash_high_nibble_difference__expected)
{
    constexpr hash_digest hash1{ 0x00 };
    constexpr hash_digest hash2{ 0x10 };
    const history a{ { hash1, 0 }, 0, history::unconfirmed_position };
    const history b{ { hash2, 0 }, 0, history::unconfirmed_position };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(history__less_than__unconfirmed_hash_low_nibble_difference__expected)
{
    constexpr hash_digest hash1{ 0x00 };
    constexpr hash_digest hash2{ 0x01 };
    const history a{ { hash1, 0 }, 0, history::unconfirmed_position };
    const history b{ { hash2, 0 }, 0, history::unconfirmed_position };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(history__less_than__unconfirmed_hash_identical__expected)
{
    constexpr hash_digest hash{ 0x00 };
    const history value{ { hash, 0 }, 0, 0 };
    BOOST_REQUIRE(!(value < value));
}

BOOST_AUTO_TEST_CASE(history__equality__distinct__false)
{
    constexpr hash_digest hash1{ 0x00 };
    constexpr hash_digest hash2{ 0x01 };
    const history a{ { hash1, 0 }, 0, history::unconfirmed_position };
    const history b{ { hash2, 0 }, 0, history::unconfirmed_position };
    BOOST_REQUIRE(!(a == b));
    BOOST_REQUIRE(!(b == a));
}

BOOST_AUTO_TEST_CASE(history__equality__same__true)
{
    constexpr hash_digest hash{ 0x01 };
    const history a{ { hash, 1 }, 2, 3 };
    BOOST_REQUIRE(a == a);
}

// filter_sort_and_dedup

BOOST_AUTO_TEST_CASE(history__filter_sort_and_dedup__unsorted_with_duplicates_mixed__sorted_and_deduped)
{
    constexpr hash_digest h1{ 0x01 };
    constexpr hash_digest h2{ 0x02 };
    std::vector<history> values
    {
        { { h2,              0 }, 0, history::unconfirmed_position },   // unconfirmed
        { { hash_digest{}, 200 }, 0, 5 },                               // confirmed
        { { h1,              0 }, 0, history::unconfirmed_position },   // unconfirmed (duplicate will be removed)
        { { h1,              0 }, 0, history::unconfirmed_position },   // unconfirmed duplicate
        { { hash_digest{}, 100 }, 0, 10 },                              // confirmed
        { { hash_digest{}, 100 }, 0, 10 }                               // confirmed duplicate
    };

    history::filter_sort_and_dedup(values);
    BOOST_REQUIRE_EQUAL(values.size(), 4u);
    BOOST_REQUIRE_EQUAL(values[0].tx.height(), 100u);                   // confirmed, lowest height
    BOOST_REQUIRE_EQUAL(values[1].tx.height(), 200u);                   // confirmed
    BOOST_REQUIRE_EQUAL(values[2].tx.height(), 0u);                     // unconfirmed (h1)
    BOOST_REQUIRE_EQUAL(values[3].tx.height(), 0u);                     // unconfirmed (h2)
}

BOOST_AUTO_TEST_CASE(history__filter_sort_and_dedup__exclusions__removes_excluded_items)
{
    std::vector<history> items
    {
        history{ checkpoint{}, 0, history::unconfirmed_position },      // excluded (default checkpoint)
        history{ checkpoint{}, 0, history::unconfirmed_position },      // excluded (default checkpoint)
        history{ { hash_digest{}, 3 }, 0, 10 },                         // valid
        history{ { hash_digest{}, 3 }, 0,  5 },                         // valid (same height, lower position)
        history{ { hash_digest{}, 3 }, 0, 10 }                          // duplicate
    };

    history::filter_sort_and_dedup(items);
    BOOST_REQUIRE_EQUAL(items.size(), 2u);
    BOOST_REQUIRE_EQUAL(items[0].position, 5u);
    BOOST_REQUIRE_EQUAL(items[1].position, 10u);
}

BOOST_AUTO_TEST_SUITE_END()
