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

BOOST_FIXTURE_TEST_SUITE(query_batch_silent_tests, test::directory_setup_fixture)

// set_silent
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__coinbase_only__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(0, test::genesis));
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__unarchived_block__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.set_silent(1, test::block_coinbase_spend_1a));
}

BOOST_AUTO_TEST_SUITE_END()
