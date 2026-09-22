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

BOOST_FIXTURE_TEST_SUITE(query_navigate_tests, test::directory_setup_fixture)

// top_header
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_navigate__top_header__genesis_bucket__genesis_link)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.header.buckets = 1;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.top_header(0), 0u);
}

BOOST_AUTO_TEST_CASE(query_navigate__top_header__empty_bucket__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.header.buckets = 8;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.top_header(7).is_terminal());
}

// top_tx
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_navigate__top_tx__empty_bucket__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.tx.buckets = 8;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.top_tx(7).is_terminal());
}

BOOST_AUTO_TEST_CASE(query_navigate__top_tx__genesis_bucket__genesis_tx)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.tx.buckets = 1;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.top_tx(0), 0u);
}

// top_point
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_navigate__top_point__empty_bucket__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.ins.buckets = 8;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.top_point(7).is_terminal());
}

BOOST_AUTO_TEST_SUITE_END()
