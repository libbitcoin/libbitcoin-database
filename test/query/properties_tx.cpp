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
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BC_FIXTURE_TEST_SUITE(query_properties_tx_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

BC_AUTO_TEST_CASE(query_properties_tx__get_wtxid__not_pooled__computed)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    tx_link link{};
    BC_REQUIRE(!query.set_code(link, test::tx4));
    BC_REQUIRE_EQUAL(query.get_wtxid(link), test::tx4.hash(true));
    BC_REQUIRE_NE(query.get_wtxid(link), test::tx4.hash(false));
}

BC_AUTO_TEST_CASE(query_properties_tx__get_wtxid__pooled__pool_columns)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    tx_link link{};
    BC_REQUIRE(!query.set_code(link, test::tx4));

    // Pool another tx under the link, so the result must come from the pool.
    const transaction source{ test::tx5.to_data(true), true };
    source.inputs_ptr()->front()->metadata.parent_tx = 42;
    BC_REQUIRE(query.set_pooled(link, source, context{ bip113, 8, 9 }, 0));
    BC_REQUIRE_EQUAL(query.get_wtxid(link), test::tx5.hash(true));
}

// is_coinbase

BC_AUTO_TEST_CASE(query_properties_tx__is_coinbase__coinbase__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1, context{}, {}, false, false));
    BC_CHECK(query.set(test::block2, context{}, {}, false, false));
    BC_CHECK(query.set(test::block3, context{}, {}, false, false));
    BC_CHECK(query.is_coinbase(0));
    BC_CHECK(query.is_coinbase(1));
    BC_CHECK(query.is_coinbase(2));
    BC_CHECK(query.is_coinbase(3));
}

BC_AUTO_TEST_CASE(query_properties_tx__is_coinbase__non_coinbase__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, context{}, {}, false, false));
    BC_CHECK(query.set(test::block2a, context{}, {}, false, false));
    BC_CHECK(!query.is_coinbase(1));
    BC_CHECK(!query.is_coinbase(2));
    BC_CHECK(!query.is_coinbase(3));
    BC_CHECK(!query.is_coinbase(4));
    BC_CHECK(!query.is_coinbase(5));
    BC_CHECK(!query.is_coinbase(42));
}

// is_tx_segregated

BC_AUTO_TEST_CASE(query_properties_tx__is_tx_segregated__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK_EQUAL(store.create(test::events_handler), error::success);
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, context{}, {}, false, false));
    BC_CHECK(!query.is_tx_segregated(0));
    BC_CHECK( query.is_tx_segregated(1));
    BC_CHECK(!query.is_tx_segregated(2));
}

// get_tx_height

BC_AUTO_TEST_CASE(query_properties_tx__get_tx_height__not_strong__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::tx4));

    size_t out{};
    BC_CHECK(!query.get_tx_height(out, 1));
}

// get_tx_position

BC_AUTO_TEST_CASE(query_properties_tx__get_tx_position__strong__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BC_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BC_CHECK(query.set(test::block3a, context{ 0, 3, 0 }, {}, false, false));

    size_t out{};
    const auto foo = query.get_tx_position(out, 0);
    BC_CHECK(foo);
    BC_CHECK_EQUAL(out, 0u);

    BC_CHECK(!query.get_tx_position(out, 1));
    BC_CHECK(!query.get_tx_position(out, 2));
    BC_CHECK(!query.get_tx_position(out, 3));
    BC_CHECK(!query.get_tx_position(out, 4));
    BC_CHECK(query.set_strong(1));
    BC_CHECK(query.set_strong(2));
    BC_CHECK(query.set_strong(3));

    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 0));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 1));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 2));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 3));
    BC_CHECK_EQUAL(out, 1u);
    BC_CHECK(query.get_tx_position(out, 4));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(!query.get_tx_position(out, 5));

    BC_CHECK_EQUAL(query.get_position_tx(0, 0), 0u);
    BC_CHECK_EQUAL(query.get_position_tx(1, 0), 1u);
    BC_CHECK_EQUAL(query.get_position_tx(2, 0), 2u);
    BC_CHECK_EQUAL(query.get_position_tx(2, 1), 3u);
    BC_CHECK_EQUAL(query.get_position_tx(3, 0), 4u);
}

BC_AUTO_TEST_CASE(query_properties_tx__get_tx_position__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BC_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BC_CHECK(query.set(test::block3a, context{ 0, 3, 0 }, {}, false, false));
    BC_CHECK(query.set(test::tx4));

    size_t out{};
    BC_CHECK(query.get_tx_position(out, 0));
    BC_CHECK_EQUAL(out, 0u);

    BC_CHECK(!query.get_tx_position(out, 1));
    BC_CHECK(!query.get_tx_position(out, 2));
    BC_CHECK(!query.get_tx_position(out, 3));
    BC_CHECK(!query.get_tx_position(out, 4));
    BC_CHECK(query.set_strong(1));
    BC_CHECK(query.set_strong(2));
    BC_CHECK(query.set_strong(3));

    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 0));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 1));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 2));
    BC_CHECK_EQUAL(out, 0u);
    BC_CHECK(query.get_tx_position(out, 3));
    BC_CHECK_EQUAL(out, 1u);
    BC_CHECK(query.get_tx_position(out, 4));
    BC_CHECK_EQUAL(out, 0u);

    // tx4 is unconfirmed.
    BC_CHECK(!query.get_tx_position(out, 5));
}

BC_AUTO_TEST_SUITE_END()
