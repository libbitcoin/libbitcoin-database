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
#include <boost/test/unit_test.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;
using namespace bc::system;

BOOST_AUTO_TEST_SUITE(accessor_tests)

BOOST_AUTO_TEST_CASE(accessor_constructor__always__buffer_nullptr)
{
    shared_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE(instance.buffer() == nullptr);
}

BOOST_AUTO_TEST_CASE(accessor_increment__nonzero__expected_offset)
{
    uint8_t value;
    auto buffer = &value;
    shared_mutex mutex;
    accessor instance(mutex);
    instance.assign(buffer);
    const auto offset = 42u;
    instance.increment(offset);
    BOOST_REQUIRE_EQUAL(instance.buffer(), buffer + offset);
}

BOOST_AUTO_TEST_CASE(accessor_assign__nonzero__expected_buffer)
{
    uint8_t value;
    auto expected = &value;
    shared_mutex mutex;
    accessor instance(mutex);
    instance.assign(expected);
    BOOST_REQUIRE_EQUAL(instance.buffer(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
