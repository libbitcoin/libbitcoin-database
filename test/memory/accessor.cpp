/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_CASE(accessor__construct_shared_mutex__unassigned__null_data)
{
    std::shared_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE(is_null(instance.begin()));
    BOOST_REQUIRE(is_null(instance.end()));
}

BOOST_AUTO_TEST_CASE(accessor__construct_upgrade_mutex__unassigned__null_data)
{
    boost::upgrade_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE(is_null(instance.begin()));
    BOOST_REQUIRE(is_null(instance.end()));
}

BOOST_AUTO_TEST_CASE(accessor__assign__data__expected)
{
    // chunk.data() is nullptr if chunk.empty(), which triggers assertion.
    data_chunk chunk{ 0x00 };
    std::shared_mutex mutex;
    accessor instance(mutex);
    const auto expected_begin = chunk.data();
    const auto expected_end = std::next(expected_begin, 1u);
    instance.assign(expected_begin, expected_end);
    BOOST_REQUIRE_EQUAL(instance.begin(), expected_begin);
    BOOST_REQUIRE_EQUAL(instance.end(), expected_end);
}

BOOST_AUTO_TEST_CASE(accessor__increment__nonzero__expected_offset)
{
    constexpr auto offset = 42u;
    data_chunk chunk(add1(offset), 0x00);
    chunk[offset] = 0xff;
    std::shared_mutex mutex;
    accessor instance(mutex);
    const auto buffer = chunk.data();
    instance.assign(buffer, std::next(buffer, offset));
    instance.increment(offset);
    BOOST_REQUIRE_EQUAL(*instance.begin(), chunk[offset]);
    BOOST_REQUIRE_EQUAL(instance.begin(), std::next(buffer, offset));
}

BOOST_AUTO_TEST_CASE(accessor__destruct__shared_lock__released)
{
    boost::upgrade_mutex mutex;
    auto access = std::make_shared<accessor<boost::upgrade_mutex>>(mutex);
    BOOST_REQUIRE(!mutex.try_lock());
    access.reset();
    BOOST_REQUIRE(mutex.try_lock());
}

BOOST_AUTO_TEST_CASE(accessor__cast__data_slab__expected)
{
    data_chunk chunk{ 0x00 };
    std::shared_mutex mutex;
    accessor instance(mutex);
    auto expected_begin = chunk.data();
    auto expected_end = std::next(expected_begin, 1u);
    instance.assign(expected_begin, expected_end);
    const auto slab = static_cast<system::data_slab>(instance);
    BOOST_REQUIRE_EQUAL(slab.begin(), expected_begin);
    BOOST_REQUIRE_EQUAL(slab.end(), expected_end);
}

BOOST_AUTO_TEST_CASE(accessor__cast__data_reference__expected)
{
    data_chunk chunk{ 0x00 };
    std::shared_mutex mutex;
    accessor instance(mutex);
    auto expected_begin = chunk.data();
    auto expected_end = std::next(expected_begin, 1u);
    instance.assign(expected_begin, expected_end);
    const auto reference = static_cast<system::data_reference>(instance);
    BOOST_REQUIRE_EQUAL(reference.begin(), expected_begin);
    BOOST_REQUIRE_EQUAL(reference.end(), expected_end);
}

BOOST_AUTO_TEST_SUITE_END()
