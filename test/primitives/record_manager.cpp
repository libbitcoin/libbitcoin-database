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

#include <bitcoin/database.hpp>
#include "../utility/storage.hpp"

using namespace bc;
using namespace bc::database;

BOOST_AUTO_TEST_SUITE(record_manager_tests)

BOOST_AUTO_TEST_CASE(record_manager__method__vector__expectation)
{
    test::storage file;
    BOOST_REQUIRE(file.open());

    record_manager<uint32_t> manager(file, 0, 10);
    BOOST_REQUIRE(manager.create());

    const auto index1 = manager.allocate(1);
    BOOST_REQUIRE_EQUAL(index1, 0u);

    const auto index2 = manager.allocate(1);
    BOOST_REQUIRE_EQUAL(index2, 1u);
    BOOST_REQUIRE_GE(file.size(),  2u * 10u + 4u);
    manager.commit();
}

BOOST_AUTO_TEST_SUITE_END()
