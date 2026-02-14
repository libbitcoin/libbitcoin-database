/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_FIXTURE_TEST_SUITE(query_height_tests, test::directory_setup_fixture)

const auto events_handler = [](auto, auto) {};

// get_candidate_hashes

BOOST_AUTO_TEST_CASE(query_height__get_candidate_hashes__initialized__one)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_candidate_hashes({ 0, 1, 2, 4, 6, 8 }).size(), 1u);
}

BOOST_AUTO_TEST_CASE(query_height__get_candidate_hashes__gapped__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3.hash())));
    const auto locator = query.get_candidate_hashes({ 0, 1, 3, 4 });
    BOOST_REQUIRE_EQUAL(locator.size(), 3u);
    BOOST_REQUIRE_EQUAL(locator[0], test::genesis.hash());
    BOOST_REQUIRE_EQUAL(locator[1], test::block1.hash());
    BOOST_REQUIRE_EQUAL(locator[2], test::block3.hash());
}

// get_confirmed_hashes1

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_hashes1__initialized__one)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes({ 0, 1, 2, 4, 6, 8 }).size(), 1u);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_hashes1__gapped__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));
    const auto locator = query.get_confirmed_hashes({ 0, 1, 3, 4 });
    BOOST_REQUIRE_EQUAL(locator.size(), 3u);
    BOOST_REQUIRE_EQUAL(locator[0], test::genesis.hash());
    BOOST_REQUIRE_EQUAL(locator[1], test::block1.hash());
    BOOST_REQUIRE_EQUAL(locator[2], test::block3.hash());
}

// get_confirmed_hashes2

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_hashes2__various__expected_sizes)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 3).size(), 3u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 4).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 3).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 2).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(3, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(3, 1).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(4, 0).size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_hashes2__three__ascending_order)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));

    const auto hashes = query.get_confirmed_hashes(0, 3);
    BOOST_REQUIRE_EQUAL(hashes.size(), 3);
    BOOST_REQUIRE_EQUAL(hashes[0], test::block1.hash());
    BOOST_REQUIRE_EQUAL(hashes[1], test::block2.hash());
    BOOST_REQUIRE_EQUAL(hashes[2], test::block3.hash());
}

BOOST_AUTO_TEST_SUITE_END()
