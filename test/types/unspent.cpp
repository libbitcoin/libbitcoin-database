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

BOOST_AUTO_TEST_SUITE(unspent_tests)

using namespace system;

// less_than.operator()

BOOST_AUTO_TEST_CASE(unspent__less_than__confirmed_before_unconfirmed__expected)
{
    const unspent a{ {}, 42, 7 };
    const unspent b{ {}, 0, unspent::unconfirmed_position };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(unspent__less_than__confirmed_height_ascending__expected)
{
    const unspent a{ {}, 100, 5 };
    const unspent b{ {}, 200, 5 };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(unspent__less_than__confirmed_position_ascending__expected)
{
    const unspent a{ {}, 100, 3 };
    const unspent b{ {}, 100, 10 };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(unspent__less_than__confirmed_output_index_ascending__expected)
{
    const outpoint p1{ { {}, 0 }, 42 };
    const outpoint p2{ { {}, 5 }, 42 };
    const unspent a{ p1, 100, 10 };
    const unspent b{ p2, 100, 10 };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(unspent__less_than__unconfirmed_outpoint_ascending__expected)
{
    const outpoint p1{ { {}, 3 }, 42 };
    const outpoint p2{ { {}, 8 }, 42 };
    const unspent a{ p1, 0, unspent::unconfirmed_position };
    const unspent b{ p2, 0, unspent::unconfirmed_position };
    BOOST_REQUIRE(a < b);
    BOOST_REQUIRE(!(b < a));
}

BOOST_AUTO_TEST_CASE(unspent__equality__distinct__false)
{
    const outpoint p1{ { {}, 0 }, 42 };
    const outpoint p2{ { {}, 1 }, 42 };
    const unspent a{ p1, 101, 10 };
    const unspent b{ p2, 100, 11 };
    BOOST_REQUIRE(!(a == b));
    BOOST_REQUIRE(!(b == a));
}

BOOST_AUTO_TEST_CASE(unspent__equality__same__true)
{
    const outpoint point{ { {}, 0 }, 42 };
    const unspent a{ point, 100, 10 };
    BOOST_REQUIRE(a == a);
}

// filter_sort_and_dedup

BOOST_AUTO_TEST_CASE(unspent__filter_sort_and_dedup__unsorted_with_duplicates_mixed__sorted_and_deduped)
{
    const outpoint lo{ { {}, 0 }, 42 };
    const outpoint hi{ { {}, 5 }, 42 };
    std::vector<unspent> values
    {
        { hi, 0, unspent::unconfirmed_position },   // unconfirmed
        { lo, 200, 3 },                             // confirmed
        { lo, 100, 5 },                             // confirmed
        { lo, 100, 5 },                             // confirmed duplicate
        { hi, 0, unspent::unconfirmed_position }    // unconfirmed duplicate
    };

    unspent::filter_sort_and_dedup(values);
    BOOST_REQUIRE_EQUAL(values.size(), 3u);
    BOOST_REQUIRE_EQUAL(values[0].height, 100u);                    // confirmed, lowest height
    BOOST_REQUIRE_EQUAL(values[1].height, 200u);                    // confirmed
    BOOST_REQUIRE_EQUAL(values[2].height, unspent::unused_height);  // unconfirmed
}

BOOST_AUTO_TEST_CASE(unspent__filter_sort_and_dedup__exclusions__removes_excluded_items)
{
    unspents items
    {
        unspent{ outpoint{},  10, 42 },             // excluded (default outpoint)
        unspent{ outpoint{}, 200, 42 },             // excluded (default outpoint)
        unspent{ { {}, 3 },   50, 10 },             // valid confirmed
        unspent{ { {}, 4 },   50,  5 },             // valid confirmed (same height, lower position)
        unspent{ { {}, 3 },   50, 10 }              // duplicate
    };

    unspent::filter_sort_and_dedup(items);
    BOOST_REQUIRE_EQUAL(items.size(), 2u);
    BOOST_REQUIRE_EQUAL(items[0].position, 5u);
    BOOST_REQUIRE_EQUAL(items[1].position, 10u);
}

BOOST_AUTO_TEST_SUITE_END()
