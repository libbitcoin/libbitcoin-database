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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

// set_prevouts
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__set_prevouts__coinbase_only__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_prevouts(1, test::block1));
}

BOOST_AUTO_TEST_CASE(query_consensus__set_prevouts__spending_block__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_prevouts(2, test::block_spend_1a));
}

BOOST_AUTO_TEST_CASE(query_consensus__set_prevouts__coinbase_and_spend__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.to_spending_txs(2).empty());
    BOOST_REQUIRE(query.set_prevouts(2, test::block_coinbase_spend_1a));
}

// get_prevouts (via block_confirmable)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__get_prevouts__cached__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_prevouts(2, test::block_coinbase_spend_1a));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_prevouts__populated_metadata__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.populate_with_metadata(test::block_coinbase_spend_1a));
    BOOST_REQUIRE(query.set_prevouts(2, test::block_coinbase_spend_1a));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_prevouts__not_cached__integrity_get_prevouts)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_get_prevouts);
}

BOOST_AUTO_TEST_SUITE_END()
