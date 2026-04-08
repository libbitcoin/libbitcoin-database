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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(types_tests)

using namespace system;

// span.size

BOOST_AUTO_TEST_CASE(types__span_size__default__zero)
{
    const span instance{};
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
}

BOOST_AUTO_TEST_CASE(types__span_size__empty_range__zero)
{
    const span instance{ 5, 5 };
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
}

BOOST_AUTO_TEST_CASE(types__span_size__non_empty_range__expected)
{
    const span instance{ 10, 25 };
    BOOST_REQUIRE_EQUAL(instance.size(), 15u);
}

BOOST_AUTO_TEST_CASE(types__span_size__negative_range__zero)
{
    const span instance{ 100, 30 };
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
}

// unspent.less_than.operator()

BOOST_AUTO_TEST_CASE(types__unspent_less_than__confirmed_before_unconfirmed__expected)
{
    const unspent a{ {}, 42, 7 };
    const unspent b{ {}, 0, 0 };
    BOOST_REQUIRE( unspent::less_than{}(a, b));
    BOOST_REQUIRE(!unspent::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__unspent_less_than__confirmed_height_ascending__expected)
{
    const unspent a{ {}, 100, 5 };
    const unspent b{ {}, 200, 5 };
    BOOST_REQUIRE( unspent::less_than{}(a, b));
    BOOST_REQUIRE(!unspent::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__unspent_less_than__confirmed_position_ascending__expected)
{
    const unspent a{ {}, 100, 3 };
    const unspent b{ {}, 100, 10 };
    BOOST_REQUIRE(unspent::less_than{}(a, b));
}

BOOST_AUTO_TEST_CASE(types__unspent_less_than__confirmed_output_index_ascending__expected)
{
    const outpoint p1{ { {}, 0 }, 0 };
    const outpoint p2{ { {}, 5 }, 0 };
    const unspent a{ p1, 100, 10 };
    const unspent b{ p2, 100, 10 };
    BOOST_REQUIRE( unspent::less_than{}(a, b));
    BOOST_REQUIRE(!unspent::less_than{}(b, b));
}

BOOST_AUTO_TEST_CASE(types__unspent_less_than__unconfirmed_outpoint_ascending__expected)
{
    const outpoint p1{ { {}, 3 }, 0 };
    const outpoint p2{ { {}, 8 }, 0 };
    const unspent a{ p1, 0, 0 };
    const unspent b{ p2, 0, 0 };
    BOOST_REQUIRE( unspent::less_than{}(a, b));
    BOOST_REQUIRE(!unspent::less_than{}(b, a));
}

// history.less_than.operator()

BOOST_AUTO_TEST_CASE(types__history_less_than__confirmed_before_unconfirmed__expected)
{
    const history a{ { hash_digest{}, 42 }, 0, 7 };
    const history b{ { hash_digest{}, 0 }, 0, 0 };
    BOOST_REQUIRE( history::less_than{}(a, b));
    BOOST_REQUIRE(!history::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__history_less_than__confirmed_height_ascending__expected)
{
    const history a{ { hash_digest{}, 100 }, 0, 5 };
    const history b{ { hash_digest{}, 200 }, 0, 5 };
    BOOST_REQUIRE( history::less_than{}(a, b));
    BOOST_REQUIRE(!history::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__history_less_than__confirmed_position_ascending__expected)
{
    const history a{ { hash_digest{}, 100 }, 0, 3 };
    const history b{ { hash_digest{}, 100 }, 0, 10 };
    BOOST_REQUIRE( history::less_than{}(a, b));
    BOOST_REQUIRE(!history::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__history_less_than__unconfirmed_hash_high_nibble_difference__expected)
{
    constexpr auto hash1 = base16_hash("0000000000000000000000000000000000000000000000000000000000000000");
    constexpr auto hash2 = base16_hash("0000000000000000000000000000000000000000000000000000000000000010");
    const history a{ { hash1, 0 }, 0, 0 };
    const history b{ { hash2, 0 }, 0, 0 };
    BOOST_REQUIRE( history::less_than{}(a, b));
    BOOST_REQUIRE(!history::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__history_less_than__unconfirmed_hash_low_nibble_difference__expected)
{
    constexpr auto hash1 = base16_hash("0000000000000000000000000000000000000000000000000000000000000000");
    constexpr auto hash2 = base16_hash("0000000000000000000000000000000000000000000000000000000000000001");
    const history a{ { hash1, 0 }, 0, 0 };
    const history b{ { hash2, 0 }, 0, 0 };
    BOOST_REQUIRE( history::less_than{}(a, b));
    BOOST_REQUIRE(!history::less_than{}(b, a));
}

BOOST_AUTO_TEST_CASE(types__history_less_than__unconfirmed_hash_identical__expected)
{
    constexpr auto hash = base16_hash("0000000000000000000000000000000000000000000000000000000000000000");
    const history value{ { hash, 0 }, 0, 0 };
    BOOST_REQUIRE(!history::less_than{}(value, value));
}

BOOST_AUTO_TEST_SUITE_END()
