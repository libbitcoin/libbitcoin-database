/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>
#include "../utility/utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;

#define DIRECTORY "record_manager"

struct record_manager_directory_setup_fixture
{
    record_manager_directory_setup_fixture()
    {
        BOOST_REQUIRE(test::clear_path(DIRECTORY));
    }
};

BOOST_FIXTURE_TEST_SUITE(hash_table_tests, record_manager_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(record_manager__method__vector__expectation)
{
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_CASE(record_manager__test)
{
    test::create(DIRECTORY "/record_manager");
    file_map file(DIRECTORY "/record_manager");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.access()->buffer() != nullptr);
    file.resize(4);

    record_manager recs(file, 0, 10);
    BOOST_REQUIRE(recs.create());
    BOOST_REQUIRE(recs.start());

    array_index idx = recs.new_records(1);
    BOOST_REQUIRE(idx == 0);
    idx = recs.new_records(1);
    BOOST_REQUIRE(idx == 1);
    BOOST_REQUIRE(file.size() >= 2 * 10 + 4);
    recs.sync();
}

BOOST_AUTO_TEST_SUITE_END()
