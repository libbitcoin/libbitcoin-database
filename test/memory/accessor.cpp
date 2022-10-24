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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(accessor_tests)

BOOST_AUTO_TEST_CASE(accessor__construct_upgrade_mutex__unassigned__null_data)
{
    boost::upgrade_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE(is_null(instance.data()));
}

BOOST_AUTO_TEST_CASE(accessor__construct_shared_mutex__unassigned__null_data)
{
    std::shared_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE(is_null(instance.data()));
}

BOOST_AUTO_TEST_CASE(accessor__assign__data__expected)
{
    // chunk.data() is nullptr if chunk.empty(), which triggers assertion.
    data_chunk chunk{ 0x00 };
    auto expected = chunk.data();
    std::shared_mutex mutex;
    accessor instance(mutex);
    instance.assign(expected);
    BOOST_REQUIRE_EQUAL(instance.data(), expected);
}

BOOST_AUTO_TEST_CASE(accessor__increment__nonzero__expected_offset)
{
    constexpr auto offset = 42u;
    data_chunk chunk(add1(offset), 0x00);
    chunk[offset] = 0xff;
    auto buffer = chunk.data();
    std::shared_mutex mutex;
    accessor instance(mutex);
    instance.assign(buffer);
    instance.increment(offset);
    BOOST_REQUIRE_EQUAL(*instance.data(), chunk[offset]);
    BOOST_REQUIRE_EQUAL(instance.data(), std::next(buffer, offset));
}

BOOST_AUTO_TEST_CASE(accessor__destruct__shared_lock__released)
{
    boost::upgrade_mutex mutex;
    auto access = std::make_shared<accessor<boost::upgrade_mutex>>(mutex);
    BOOST_REQUIRE(!mutex.try_lock());
    access.reset();
    BOOST_REQUIRE(mutex.try_lock());
}

BOOST_AUTO_TEST_SUITE_END()
