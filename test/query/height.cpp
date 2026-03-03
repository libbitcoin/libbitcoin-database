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
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1_hash)));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2_hash)));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3_hash)));
    const auto locator = query.get_candidate_hashes({ 0, 1, 3, 4 });
    BOOST_REQUIRE_EQUAL(locator.size(), 3u);
    BOOST_REQUIRE_EQUAL(locator[0], test::block0_hash);
    BOOST_REQUIRE_EQUAL(locator[1], test::block1_hash);
    BOOST_REQUIRE_EQUAL(locator[2], test::block3_hash);
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
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3_hash), false));
    const auto locator = query.get_confirmed_hashes({ 0, 1, 3, 4 });
    BOOST_REQUIRE_EQUAL(locator.size(), 3u);
    BOOST_REQUIRE_EQUAL(locator[0], test::block0_hash);
    BOOST_REQUIRE_EQUAL(locator[1], test::block1_hash);
    BOOST_REQUIRE_EQUAL(locator[2], test::block3_hash);
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
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3_hash), false));
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 3).size(), 3u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 4).size(), 4u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(0, 5).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 3).size(), 3u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(1, 4).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(2, 3).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(3, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(3, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_hashes(3, 2).size(), 0u);
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
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3_hash), false));

    const auto hashes = query.get_confirmed_hashes(0, 3);
    BOOST_REQUIRE_EQUAL(hashes.size(), 3);
    BOOST_REQUIRE_EQUAL(hashes[0], test::block0_hash);
    BOOST_REQUIRE_EQUAL(hashes[1], test::block1_hash);
    BOOST_REQUIRE_EQUAL(hashes[2], test::block2_hash);
}

// get_confirmed_headers

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_headers__empty__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 42).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(42, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(42, 42).size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_headers__unconfirmeds__empty)
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
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1_hash)));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2_hash)));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3_hash)));
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 42).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(42, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(42, 42).size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_headers__confirmeds__expected_size)
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
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3_hash)));
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(0, 3).size(), 3u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(1, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(1, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(1, 2).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(1, 3).size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(2, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(2, 1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(2, 2).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(3, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(3, 1).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(4, 0).size(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_confirmed_headers(4, 1).size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_headers__confirmeds__expected_headers)
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
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3_hash)));

    const auto headers = query.get_confirmed_headers(1, 3);
    BOOST_REQUIRE_EQUAL(headers.size(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[0]), test::block1_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[1]), test::block2_hash);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_headers__over_top__reduced_count)
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
    BOOST_REQUIRE(query.set(test::block4, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block5, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block6, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block7, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block8, test::context, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block4_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block5_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block6_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block7_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block8_hash), false));

    // limit = min(1000, add1(8) - 3) = 6
    // last  = 3 + sub1(6) = 8
    // first = 3
    // count = add1(8 - 3) = 6
    const auto headers = query.get_confirmed_headers(3, 1000);
    BOOST_REQUIRE_EQUAL(headers.size(), 6u);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[0]), test::block3_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[1]), test::block4_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[2]), test::block5_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[3]), test::block6_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[4]), test::block7_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[5]), test::block8_hash);
}

BOOST_AUTO_TEST_CASE(query_height__get_confirmed_headers__under_top__full_count)
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
    BOOST_REQUIRE(query.set(test::block4, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block5, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block6, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block7, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block8, test::context, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block4_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block5_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block6_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block7_hash), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block8_hash), false));

    const auto headers = query.get_confirmed_headers(2, 6);
    BOOST_REQUIRE_EQUAL(headers.size(), 6u);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[0]), test::block2_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[1]), test::block3_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[2]), test::block4_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[3]), test::block5_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[4]), test::block6_hash);
    BOOST_REQUIRE_EQUAL(query.get_header_key(headers[5]), test::block7_hash);
}

BOOST_AUTO_TEST_SUITE_END()
