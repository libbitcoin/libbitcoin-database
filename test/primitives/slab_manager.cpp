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
#include "../utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;

#define DIRECTORY "slab_manager"

class slab_manager_directory_setup_fixture
{
public:
    slab_manager_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
    }
};

BOOST_FIXTURE_TEST_SUITE(slab_manager_tests, slab_manager_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(slab_manager__method__vector__expectation)
{
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_CASE(slab_manager__test)
{
    test::create(DIRECTORY "/slab_manager");
    memory_map file(DIRECTORY "/slab_manager");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.access()->buffer() != nullptr);
    file.resize(200);

    slab_manager data(file, 0);
    BOOST_REQUIRE(data.create());
    BOOST_REQUIRE(data.start());

    file_offset position = data.new_slab(100);
    BOOST_REQUIRE(position == 8);
    //slab_byte_pointer slab = data.get(position);

    file_offset position2 = data.new_slab(100);
    BOOST_REQUIRE(position2 == 108);
    //slab = data.get(position2);

    BOOST_REQUIRE(file.size() >= 208);
}

BOOST_AUTO_TEST_SUITE_END()
