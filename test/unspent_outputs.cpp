/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <boost/test/unit_test.hpp>

#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::chain;
using namespace bc::database;

// TODO: test with confirmed/unconfirmed.

BOOST_AUTO_TEST_SUITE(unspent_outputs_tests)

BOOST_AUTO_TEST_CASE(unspent_outputs__construct__capacity_0__disabled)
{
    const unspent_outputs cache(0);
    BOOST_REQUIRE(cache.disabled());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__construct__capacity_42__not_disabled)
{
    const unspent_outputs cache(42);
    BOOST_REQUIRE(!cache.disabled());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__construct__capacity_0__size_0)
{
    const unspent_outputs cache(0);
    BOOST_REQUIRE_EQUAL(cache.size(), 0u);
}

BOOST_AUTO_TEST_CASE(unspent_outputs__construct__capacity_42__empty)
{
    const unspent_outputs cache(42);
    BOOST_REQUIRE(cache.empty());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__hit_rate__default__1)
{
    const unspent_outputs cache(0);
    BOOST_REQUIRE_EQUAL(cache.hit_rate(), 1.0f);
}

BOOST_AUTO_TEST_CASE(unspent_outputs__add__one_capacity_42__size_1)
{
    static const transaction tx{ 0, 0, input::list{}, output::list{ output{} } };
    unspent_outputs cache(42);
    cache.add(tx, 0, false);
    BOOST_REQUIRE_EQUAL(cache.size(), 1u);
}

BOOST_AUTO_TEST_CASE(unspent_outputs__add__no_outputs_capcity_42__empty)
{
    static const transaction tx{ 0, 0, {}, {} };
    unspent_outputs cache(42);
    cache.add(tx, 0, false);
    BOOST_REQUIRE(cache.empty());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__add__one_capcity_0__empty)
{
    static const transaction tx{ 0, 0, {}, { {}, {} } };
    unspent_outputs cache(0);
    cache.add(tx, 0, false);
    BOOST_REQUIRE(cache.empty());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__remove1__remove_only__empty)
{
    static const transaction tx{ 0, 0, {}, { {}, {} } };
    unspent_outputs cache(1);
    cache.add(tx, 0, false);
    BOOST_REQUIRE_EQUAL(cache.size(), 1u);

    cache.remove(tx.hash());
    BOOST_REQUIRE(cache.empty());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__remove1__capcity_0__empty)
{
    unspent_outputs cache(0);
    cache.remove({ null_hash, 42 });
    BOOST_REQUIRE(cache.empty());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__remove2__capcity_0__empty)
{
    unspent_outputs cache(0);
    cache.remove(null_hash);
    BOOST_REQUIRE(cache.empty());
}

BOOST_AUTO_TEST_CASE(unspent_outputs__remove2__remove_one_output__expected_outputs)
{
    static const size_t expected_height = 41;
    static const uint64_t expected_value = 42;
    static const transaction tx1{ 0, 0, {}, { { 0, {} }, { 1, {} } } };
    static const transaction tx2{ 0, 0, {}, { { 0, {} }, { expected_value, {} } } };
    unspent_outputs cache(42);
    cache.add(tx1, 0, false);
    cache.add(tx2, expected_height, false);
    BOOST_REQUIRE_EQUAL(cache.size(), 2u);

    bool out_coinbase;
    size_t out_height;
    chain::output out_value;
    BOOST_REQUIRE(cache.get(out_value, out_height, out_coinbase, { tx1.hash(), 0 }, max_size_t, false));
    BOOST_REQUIRE(cache.get(out_value, out_height, out_coinbase, { tx1.hash(), 1 }, max_size_t, false));
    BOOST_REQUIRE(cache.get(out_value, out_height, out_coinbase, { tx2.hash(), 0 }, max_size_t, false));
    BOOST_REQUIRE(cache.get(out_value, out_height, out_coinbase, { tx2.hash(), 1 }, max_size_t, false));
    BOOST_REQUIRE(out_value.is_valid());
    BOOST_REQUIRE_EQUAL(out_coinbase, false);
    BOOST_REQUIRE_EQUAL(out_height, expected_height);
    BOOST_REQUIRE_EQUAL(out_value.value(), expected_value);

    cache.remove({ tx1.hash(), 1 });
    BOOST_REQUIRE_EQUAL(cache.size(), 2u);

    cache.remove({ tx1.hash(), 0 });
    BOOST_REQUIRE_EQUAL(cache.size(), 1u);
    BOOST_REQUIRE(!cache.get(out_value, out_height, out_coinbase, { tx1.hash(), 0 }, max_size_t, false));
    BOOST_REQUIRE(!cache.get(out_value, out_height, out_coinbase, { tx1.hash(), 1 }, max_size_t, false));
    BOOST_REQUIRE(cache.get(out_value, out_height, out_coinbase, { tx2.hash(), 0 }, max_size_t, false));
    BOOST_REQUIRE(cache.get(out_value, out_height, out_coinbase, { tx2.hash(), 1 }, max_size_t, false));
}

BOOST_AUTO_TEST_CASE(unspent_outputs__get__two_capacity_1__size_1_expected)
{
    static const size_t expected_height = 40;
    static const transaction tx1{ 0, 0, {}, { {}, {} } };
    unspent_outputs cache(1);
    cache.add(tx1, expected_height, false);
    BOOST_REQUIRE_EQUAL(cache.size(), 1u);

    bool out_coinbase;
    size_t out_height;
    chain::output out_value1;
    BOOST_REQUIRE(cache.get(out_value1, out_height, out_coinbase, { tx1.hash(), 1 }, max_size_t, false));
    BOOST_REQUIRE_EQUAL(out_coinbase, false);
    BOOST_REQUIRE_EQUAL(out_height, expected_height);
    BOOST_REQUIRE(!out_value1.is_valid());

    static const uint64_t expected2a = 41;
    static const uint64_t expected2b = 42;
    static const transaction tx2{ 0, 0, {}, { {}, { expected2a, {} }, { expected2b, {} } } };
    BOOST_REQUIRE(tx2.is_valid());

    cache.add(tx2, 0, false);
    BOOST_REQUIRE_EQUAL(cache.size(), 1u);

    chain::output out_value2a;
    BOOST_REQUIRE(cache.get(out_value2a, out_height, out_coinbase, { tx2.hash(), 1 }, max_size_t, false));
    BOOST_REQUIRE(out_value2a.is_valid());
    BOOST_REQUIRE_EQUAL(out_value2a.value(), expected2a);

    chain::output out_value2b;
    BOOST_REQUIRE(cache.get(out_value2b, out_height, out_coinbase, { tx2.hash(), 2 }, max_size_t, false));
    BOOST_REQUIRE(out_value2b.is_valid());
    BOOST_REQUIRE_EQUAL(out_value2b.value(), expected2b);
}

BOOST_AUTO_TEST_SUITE_END()
