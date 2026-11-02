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

BC_AUTO_TEST_SUITE(unspent_output_tests)

using namespace system;

// less_than.operator()

BC_AUTO_TEST_CASE(unspent_output__less_than__confirmed_before_unconfirmed__expected)
{
    const unspent_output a{ {}, 42, 7 };
    const unspent_output b{ {}, 0, unspent_output::unconfirmed_position };
    BC_REQUIRE(a < b);
    BC_REQUIRE(!(b < a));
}

BC_AUTO_TEST_CASE(unspent_output__less_than__confirmed_height_ascending__expected)
{
    const unspent_output a{ {}, 100, 5 };
    const unspent_output b{ {}, 200, 5 };
    BC_REQUIRE(a < b);
    BC_REQUIRE(!(b < a));
}

BC_AUTO_TEST_CASE(unspent_output__less_than__confirmed_position_ascending__expected)
{
    const unspent_output a{ {}, 100, 3 };
    const unspent_output b{ {}, 100, 10 };
    BC_REQUIRE(a < b);
    BC_REQUIRE(!(b < a));
}

BC_AUTO_TEST_CASE(unspent_output__less_than__confirmed_output_index_ascending__expected)
{
    const outpoint p1{ { {}, 0 }, 42 };
    const outpoint p2{ { {}, 5 }, 42 };
    const unspent_output a{ p1, 100, 10 };
    const unspent_output b{ p2, 100, 10 };
    BC_REQUIRE(a < b);
    BC_REQUIRE(!(b < a));
}

BC_AUTO_TEST_CASE(unspent_output__less_than__unconfirmed_outpoint_ascending__expected)
{
    const outpoint p1{ { {}, 3 }, 42 };
    const outpoint p2{ { {}, 8 }, 42 };
    const unspent_output a{ p1, 0, unspent_output::unconfirmed_position };
    const unspent_output b{ p2, 0, unspent_output::unconfirmed_position };
    BC_REQUIRE(a < b);
    BC_REQUIRE(!(b < a));
}

BC_AUTO_TEST_CASE(unspent_output__equality__distinct__false)
{
    const outpoint p1{ { {}, 0 }, 42 };
    const outpoint p2{ { {}, 1 }, 42 };
    const unspent_output a{ p1, 101, 10 };
    const unspent_output b{ p2, 100, 11 };
    BC_REQUIRE(!(a == b));
    BC_REQUIRE(!(b == a));
}

BC_AUTO_TEST_CASE(unspent_output__equality__same__true)
{
    const outpoint point{ { {}, 0 }, 42 };
    const unspent_output value{ point, 100, 10 };
    BC_REQUIRE(value == value);
}

BC_AUTO_TEST_CASE(unspent_output__valid__always__expected)
{
    const auto valid1 = unspent_output{ { { hash_digest{}, 0 }, 42 }, 2, 0 }.valid();
    BC_REQUIRE(valid1);

    const auto valid2 = unspent_output{ { { hash_digest{}, 0 }, 42 }, 2, unspent_output::unconfirmed_position }.valid();
    BC_REQUIRE(valid2);

    const auto invalid1 = !unspent_output{ {}, 2, 3 }.valid();
    BC_REQUIRE(invalid1);

    const auto invalid2 = !unspent_output{ {}, 2, unspent_output::unconfirmed_position }.valid();
    BC_REQUIRE(invalid2);
}

BC_AUTO_TEST_CASE(unspent_output__fault__always__expected)
{
    const auto fault1 = unspent_output{}.fault();
    BC_REQUIRE(fault1);

    const auto fault2 = unspent_output{ {}, 2, 0 }.fault();
    BC_REQUIRE(fault2);

    const auto nonfault1 = !unspent_output{ {}, 2, 3 }.fault();
    BC_REQUIRE(nonfault1);

    const auto nonfault2 = !unspent_output{ {}, 2, unspent_output::unconfirmed_position }.fault();
    BC_REQUIRE(nonfault2);
}

BC_AUTO_TEST_CASE(unspent_output__confirmed__always__expected)
{
    const auto confirmed = unspent_output{ { { hash_digest{}, 0 }, 42 }, 2, 3 }.confirmed();
    BC_REQUIRE(confirmed);

    const auto unconfirmed = !unspent_output{ { { hash_digest{}, 0 }, 42 }, 2, unspent_output::unconfirmed_position }.confirmed();
    BC_REQUIRE(unconfirmed);
}

// filter_sort_and_dedup

BC_AUTO_TEST_CASE(unspent_output__filter_sort_and_dedup__unsorted_with_duplicates_mixed__sorted_and_deduped)
{
    const outpoint lo{ { {}, 0 }, 42 };
    const outpoint hi{ { {}, 5 }, 42 };
    std::vector<unspent_output> values
    {
        { hi, 0, unspent_output::unconfirmed_position },   // unconfirmed
        { lo, 200, 3 },                             // confirmed
        { lo, 100, 5 },                             // confirmed
        { lo, 100, 5 },                             // confirmed duplicate
        { hi, 0, unspent_output::unconfirmed_position }    // unconfirmed duplicate
    };

    unspent_output::filter_sort_and_dedup(values);
    BC_REQUIRE_EQUAL(values.size(), 3u);
    BC_REQUIRE_EQUAL(values[0].height, 100u);                    // confirmed, lowest height
    BC_REQUIRE_EQUAL(values[1].height, 200u);                    // confirmed
    BC_REQUIRE_EQUAL(values[2].height, unspent_output::unused_height);  // unconfirmed
}

BC_AUTO_TEST_CASE(unspent_output__filter_sort_and_dedup__exclusions__removes_excluded_items)
{
    unspent_outputs items
    {
        unspent_output{ outpoint{},  10, 42 },             // excluded (default outpoint)
        unspent_output{ outpoint{}, 200, 42 },             // excluded (default outpoint)
        unspent_output{ { {}, 3 },   50, 10 },             // valid confirmed
        unspent_output{ { {}, 4 },   50,  5 },             // valid confirmed (same height, lower position)
        unspent_output{ { {}, 3 },   50, 10 }              // duplicate
    };

    unspent_output::filter_sort_and_dedup(items);
    BC_REQUIRE_EQUAL(items.size(), 2u);
    BC_REQUIRE_EQUAL(items[0].position, 5u);
    BC_REQUIRE_EQUAL(items[1].position, 10u);
}

BC_AUTO_TEST_SUITE_END()
