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

#include <utility>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::chain;
using namespace bc::database;

// TODO: test confirmed/is_confirmed.

BOOST_AUTO_TEST_SUITE(unspent_transaction_tests)

BOOST_AUTO_TEST_CASE(unspent_transaction__move__coinbase_tx_hash_height__expected)
{
    static const transaction tx{ 0, 0, { { { null_hash, point::null_index }, {}, 0 } }, {} };
    static const auto expected_height = 42u;
    unspent_transaction instance(tx, expected_height, 0, false);
    const unspent_transaction moved(std::move(instance));
    BOOST_REQUIRE(moved.hash() == tx.hash());
    BOOST_REQUIRE_EQUAL(moved.height(), expected_height);
    BOOST_REQUIRE_EQUAL(moved.is_coinbase(), true);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__copy__coinbase_tx_hash_height__expected)
{
    static const transaction tx{ 0, 0, { { { null_hash, point::null_index }, {}, 0 } }, {} };
    static const auto expected_height = 42u;
    const unspent_transaction instance(tx, expected_height, 0, false);
    const unspent_transaction copied(instance);
    BOOST_REQUIRE(copied.hash() == tx.hash());
    BOOST_REQUIRE_EQUAL(copied.height(), expected_height);
    BOOST_REQUIRE_EQUAL(copied.is_coinbase(), true);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__construct1__null_hash__null_hash)
{
    BOOST_REQUIRE(unspent_transaction(null_hash).hash() == null_hash);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__construct1__tx_hash__expected_hash)
{
    static const transaction tx;
    static const auto expected = tx.hash();
    BOOST_REQUIRE(unspent_transaction(expected).hash() == expected);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__construct2__point_hash__expected_hash)
{
    static const transaction tx;
    static const auto expected = tx.hash();
    static const output_point point{ expected, 42 };
    BOOST_REQUIRE(unspent_transaction(point).hash() == expected);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__construct3__tx__expected_hash)
{
    static const transaction tx;
    BOOST_REQUIRE(unspent_transaction(tx, 0, 0, false).hash() == tx.hash());
}

BOOST_AUTO_TEST_CASE(unspent_transaction__outputs__construct1__tx_hash__empty)
{
    static const transaction tx;
    const auto expected = tx.hash();
    BOOST_REQUIRE(unspent_transaction(expected).outputs()->empty());
}

BOOST_AUTO_TEST_CASE(unspent_transaction__outputs__construct2__empty)
{
    static const transaction tx;
    const output_point point{ tx.hash(), 42 };
    BOOST_REQUIRE(unspent_transaction(point).outputs()->empty());
}

BOOST_AUTO_TEST_CASE(unspent_transaction__outputs__construct3_empty_tx__empty)
{
    static const transaction tx;
    BOOST_REQUIRE(unspent_transaction(tx, 0, 0, false).outputs()->empty());
}

BOOST_AUTO_TEST_CASE(unspent_transaction__outputs__construct3_single_output_tx__one_output)
{
    static const transaction tx{ 0, 0, {}, { {} } };
    BOOST_REQUIRE_EQUAL(unspent_transaction(tx, 0, 0, false).outputs()->size(), 1u);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__equal__tx_hash_only__true)
{
    static const transaction tx;
    BOOST_REQUIRE(unspent_transaction(tx, 0, 0, false) == unspent_transaction(tx.hash()));
}

BOOST_AUTO_TEST_CASE(unspent_transaction__move_assign__coinbase_tx_hash_height__expected)
{
    static const transaction tx{ 0, 0, { { { null_hash, point::null_index }, {}, 0 } }, {} };
    static const auto expected_height = 42u;
    unspent_transaction instance(tx, expected_height, 0, false);
    const unspent_transaction moved = std::move(instance);
    BOOST_REQUIRE(moved.hash() == tx.hash());
    BOOST_REQUIRE_EQUAL(moved.height(), expected_height);
    BOOST_REQUIRE_EQUAL(moved.is_coinbase(), true);
}

BOOST_AUTO_TEST_CASE(unspent_transaction__copy_assign__coinbase_tx_hash_height__expected)
{
    static const transaction tx{ 0, 0, { { { null_hash, point::null_index }, {}, 0 } }, {} };
    static const auto expected_height = 42u;
    const unspent_transaction instance(tx, expected_height, 0, false);
    const unspent_transaction copied = instance;
    BOOST_REQUIRE(copied.hash() == tx.hash());
    BOOST_REQUIRE_EQUAL(copied.height(), expected_height);
    BOOST_REQUIRE_EQUAL(copied.is_coinbase(), true);
}

BOOST_AUTO_TEST_SUITE_END()
