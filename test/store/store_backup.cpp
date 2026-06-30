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
#include "../mocks/map_store.hpp"

BOOST_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// backup
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__backup__unloaded__backup_table)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.backup_(), error::backup_table);
}

BOOST_AUTO_TEST_CASE(store__backup__loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(!instance.backup_());
    BOOST_REQUIRE(!instance.close(test::events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
}

BOOST_AUTO_TEST_CASE(store__backup__primary_loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!instance.backup_());
    BOOST_REQUIRE(!instance.close(test::events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
}

BOOST_AUTO_TEST_CASE(store__backup__primary_secondary_loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::secondary));
    BOOST_REQUIRE(!instance.backup_());
    BOOST_REQUIRE(!instance.close(test::events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
}

BOOST_AUTO_TEST_SUITE_END()
