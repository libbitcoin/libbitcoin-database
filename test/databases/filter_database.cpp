/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

// Sponsored in part by Digital Contract Design, LLC

#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>
#include "../utility/utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;
using namespace bc::system;
using namespace bc::system::chain;

#define DIRECTORY "filter_database"
#define HASH "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"
#define HEADER "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"
#define FILTER "001a2b3c"

static BC_CONSTEXPR auto file_path = DIRECTORY "/filter_table";

struct filter_database_directory_setup_fixture
{
    filter_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }

    ~filter_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(filter_database_tests, filter_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(filter_database__store__single_filter_without_matching_type__failure)
{
    uint8_t filter_type = 0u;

    block_filter data(
        16u,
        hash_literal(HEADER),
        to_chunk(base16_literal(FILTER)));

    test::create(file_path);
    filter_database instance(file_path, 1, 1000, 50, filter_type);
    BOOST_REQUIRE(instance.create());

    const auto hash = hash_literal(HASH);
    // BOOST_REQUIRE(!instance.get(hash));

    // Setup end

    BOOST_REQUIRE_EQUAL(false, instance.store(hash, data));
    // BOOST_REQUIRE(!instance.get(hash));
}

BOOST_AUTO_TEST_CASE(filter_database__store__single_filter__success)
{
    uint8_t filter_type = 0u;

    block_filter data(
        filter_type,
        hash_literal(HEADER),
        to_chunk(base16_literal(FILTER)));

    test::create(file_path);
    filter_database instance(file_path, 1, 1000, 50, filter_type);
    BOOST_REQUIRE(instance.create());

    const auto hash = hash_literal(HASH);
    // BOOST_REQUIRE(!instance.get(hash));

    // Setup end

    BOOST_REQUIRE_EQUAL(true, instance.store(hash, data));
    BOOST_REQUIRE(data.metadata.link != block_filter::validation::unlinked);

    const auto result = instance.get(data.metadata.link);
    BOOST_REQUIRE(result);
    // BOOST_REQUIRE(result.block_hash() == hash);

    const auto block_filter = result.block_filter();
    BOOST_REQUIRE(data.filter_type() == block_filter.filter_type());
    BOOST_REQUIRE(data.header() == block_filter.header());
    BOOST_REQUIRE(data.filter() == block_filter.filter());

    const auto result2 = instance.get(result.link());
    BOOST_REQUIRE(result2);
    // BOOST_REQUIRE(result2.block_hash() == hash);
}

BOOST_AUTO_TEST_SUITE_END()
