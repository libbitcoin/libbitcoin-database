/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

using namespace system;

BOOST_AUTO_TEST_CASE(accessor__construct_shared_mutex__unassigned__nulls)
{
    std::shared_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE(is_null(instance.data()));
    BOOST_REQUIRE(is_null(instance.begin()));
    BOOST_REQUIRE(is_null(instance.end()));
}

BOOST_AUTO_TEST_CASE(accessor__destruct__shared_lock__released)
{
    std::shared_mutex mutex;
    auto access = std::make_shared<accessor<std::shared_mutex>>(mutex);
    BOOST_REQUIRE(!mutex.try_lock());
    access.reset();
    BOOST_REQUIRE(mutex.try_lock());
}

BOOST_AUTO_TEST_CASE(accessor__size__default__empty)
{
    std::shared_mutex mutex;
    accessor instance(mutex);
    BOOST_REQUIRE_EQUAL(instance.size(), system::possible_narrow_and_sign_cast<ptrdiff_t>(0));
}

BOOST_AUTO_TEST_CASE(accessor__assign__forward__positive_size)
{
    data_chunk chunk{ 0x00 };
    std::shared_mutex mutex;
    accessor instance(mutex);
    const auto expected_begin = chunk.data();
    const auto expected_end = std::next(expected_begin, chunk.size());
    instance.assign(expected_begin, expected_end);
    BOOST_REQUIRE_EQUAL(instance.data(), expected_begin);
    BOOST_REQUIRE_EQUAL(instance.begin(), expected_begin);
    BOOST_REQUIRE_EQUAL(instance.end(), expected_end);
    BOOST_REQUIRE_EQUAL(instance.size(), system::possible_narrow_and_sign_cast<ptrdiff_t>(1));
}

BOOST_AUTO_TEST_CASE(accessor__assign__reverse__negative_size)
{
    data_chunk chunk{ 0x00 };
    std::shared_mutex mutex;
    accessor instance(mutex);
    const auto expected_end = chunk.data();
    const auto expected_begin = std::next(expected_end, chunk.size());
    instance.assign(expected_begin, expected_end);
    BOOST_REQUIRE_EQUAL(instance.data(), expected_begin);
    BOOST_REQUIRE_EQUAL(instance.begin(), expected_begin);
    BOOST_REQUIRE_EQUAL(instance.end(), expected_end);
    BOOST_REQUIRE_EQUAL(instance.size(), system::possible_narrow_and_sign_cast<ptrdiff_t>(-1));
}

BOOST_AUTO_TEST_CASE(accessor__offset__valid__expected)
{
    constexpr auto offset = 42u;
    data_chunk chunk(add1(offset), 0x00);
    chunk[offset] = 0xff;
    std::shared_mutex mutex;
    accessor instance(mutex);
    const auto buffer = chunk.data();
    instance.assign(buffer, std::next(buffer, add1(offset)));
    BOOST_REQUIRE_EQUAL(*instance.offset(offset), 0xff);
}

BOOST_AUTO_TEST_CASE(accessor__offset__overflow__expected)
{
    constexpr auto offset = 42u;
    data_chunk chunk(offset, 0x00);
    std::shared_mutex mutex;
    accessor instance(mutex);
    const auto buffer = chunk.data();
    instance.assign(buffer, std::next(buffer, offset));
    BOOST_REQUIRE(is_null(instance.offset(add1(offset))));
}

BOOST_AUTO_TEST_SUITE_END()
