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

BOOST_FIXTURE_TEST_SUITE(query_filters_tests, test::directory_setup_fixture)

// filter_tx
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_filters__is_filtered_body__genesis__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.filter_enabled());
    BOOST_REQUIRE(query.is_filtered_body(0));
}

BOOST_AUTO_TEST_CASE(query_filters__is_filtered_body__unarchived__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.is_filtered_body(1));
}

BOOST_AUTO_TEST_CASE(query_filters__get_filter_body__unarchived__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    system::data_chunk out{};
    BOOST_REQUIRE(!query.get_filter_body(out, 1));
}

BOOST_AUTO_TEST_CASE(query_filters__set_filter_body__filter__round_trips)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    const auto expected = system::base16_chunk("00010203");
    BOOST_REQUIRE(query.set_filter_body(1, expected));
    BOOST_REQUIRE(query.is_filtered_body(1));
    system::data_chunk out{};
    BOOST_REQUIRE(query.get_filter_body(out, 1));
    BOOST_REQUIRE_EQUAL(out, expected);
}

// filter_bk
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_filters__is_filtered_head__genesis__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_filtered_head(0));
}

BOOST_AUTO_TEST_CASE(query_filters__is_filtered_head__unarchived__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.is_filtered_head(1));
}

BOOST_AUTO_TEST_CASE(query_filters__get_filter_head__unarchived__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    system::hash_digest out{};
    BOOST_REQUIRE(!query.get_filter_head(out, 1));
}

BOOST_AUTO_TEST_CASE(query_filters__set_filter_head__head_and_hash__round_trips)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    const auto head = system::base16_hash("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    const auto hash = system::base16_hash("2122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f40");
    BOOST_REQUIRE(query.set_filter_head(1, head, hash));
    BOOST_REQUIRE(query.is_filtered_head(1));
    system::hash_digest out{};
    BOOST_REQUIRE(query.get_filter_head(out, 1));
    BOOST_REQUIRE_EQUAL(out, head);
    system::hash_digest out_hash{};
    BOOST_REQUIRE(query.get_filter_hash(out_hash, 1));
    BOOST_REQUIRE_EQUAL(out_hash, hash);
}

BOOST_AUTO_TEST_SUITE_END()
