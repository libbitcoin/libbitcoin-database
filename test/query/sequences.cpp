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

BOOST_FIXTURE_TEST_SUITE(query_sequences_tests, test::directory_setup_fixture)

// get_thresholds
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_sequences__get_thresholds__terminal__false)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    thresholds out{};
    BOOST_REQUIRE(!query.get_thresholds(out, header_link::terminal));
}

BOOST_AUTO_TEST_CASE(query_sequences__get_thresholds__unarchived__false)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), database::context{ 0, 1, 0 }, {}, false));

    thresholds out{};
    BOOST_REQUIRE(!query.get_thresholds(out, 1));
}

BOOST_AUTO_TEST_CASE(query_sequences__get_thresholds__genesis__one)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    thresholds out{};
    BOOST_REQUIRE(query.get_thresholds(out, 0));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front().first, 0u);
    BOOST_REQUIRE_EQUAL(out.front().second, 0u);
}

BOOST_AUTO_TEST_CASE(query_sequences__get_thresholds__ordered__every_height)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, database::context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, database::context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block3, database::context{ 0, 3, 0 }, {}, false, false));

    thresholds out{};
    BOOST_REQUIRE(query.get_thresholds(out, 3));
    BOOST_REQUIRE_EQUAL(out.size(), 4u);
    BOOST_REQUIRE_EQUAL(out.at(0).first, 0u);
    BOOST_REQUIRE_EQUAL(out.at(1).first, 1u);
    BOOST_REQUIRE_EQUAL(out.at(2).first, 2u);
    BOOST_REQUIRE_EQUAL(out.at(3).first, 3u);
    BOOST_REQUIRE_EQUAL(out.at(0).second, query.to_coinbase(0));
    BOOST_REQUIRE_EQUAL(out.at(1).second, query.to_coinbase(1));
    BOOST_REQUIRE_EQUAL(out.at(2).second, query.to_coinbase(2));
    BOOST_REQUIRE_EQUAL(out.at(3).second, query.to_coinbase(3));
}

BOOST_AUTO_TEST_CASE(query_sequences__get_thresholds__disordered__lowest_above)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), database::context{ 0, 1, 0 }, {}, false));
    BOOST_REQUIRE(query.set(test::block2.header(), database::context{ 0, 2, 0 }, {}, false));
    BOOST_REQUIRE(query.set(test::block3.header(), database::context{ 0, 3, 0 }, {}, false));
    BOOST_REQUIRE(query.set(test::block3, false, false));
    BOOST_REQUIRE(query.set(test::block1, false, false));
    BOOST_REQUIRE(query.set(test::block2, false, false));
    BOOST_REQUIRE_LT(query.to_coinbase(3), query.to_coinbase(1));
    BOOST_REQUIRE_LT(query.to_coinbase(3), query.to_coinbase(2));

    thresholds out{};
    BOOST_REQUIRE(query.get_thresholds(out, 3));
    BOOST_REQUIRE_EQUAL(out.size(), 2u);
    BOOST_REQUIRE_EQUAL(out.front().first, 0u);
    BOOST_REQUIRE_EQUAL(out.front().second, 0u);
    BOOST_REQUIRE_EQUAL(out.back().first, 3u);
    BOOST_REQUIRE_EQUAL(out.back().second, query.to_coinbase(3));
}

BOOST_AUTO_TEST_SUITE_END()
