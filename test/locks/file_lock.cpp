/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_FIXTURE_TEST_SUITE(file_lock_tests, test::directory_setup_fixture)

class access final
  : public file_lock
{
public:
    using file_lock::file_lock;

    bool exists_() const NOEXCEPT
    {
        return file_lock::exists();
    }

    bool create_() NOEXCEPT
    {
        return file_lock::create();
    }

    bool destroy_() NOEXCEPT
    {
        return file_lock::destroy();
    }
};

BOOST_AUTO_TEST_CASE(file_lock__construct__file__expected)
{
    access instance(TEST_PATH);
    BOOST_REQUIRE_EQUAL(instance.file(), TEST_PATH);
}

BOOST_AUTO_TEST_CASE(file_lock__create__not_exists__true_created)
{
    BOOST_REQUIRE(!test::exists(TEST_PATH));

    access instance(TEST_PATH);
    BOOST_REQUIRE(instance.create_());
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

// Creation is idempotent.
BOOST_AUTO_TEST_CASE(file_lock__create__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));

    access instance(TEST_PATH);
    BOOST_REQUIRE(instance.create_());
}

BOOST_AUTO_TEST_CASE(file_lock__exists__not_exists__false)
{
    BOOST_REQUIRE(!test::exists(TEST_PATH));

    access instance(TEST_PATH);
    BOOST_REQUIRE(!instance.exists_());
}

BOOST_AUTO_TEST_CASE(file_lock__exists__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));

    access instance(TEST_PATH);
    BOOST_REQUIRE(instance.create_());
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

// Destruction is idempotent.
BOOST_AUTO_TEST_CASE(file_lock__destroy__not_exists__true)
{
    BOOST_REQUIRE(!test::exists(TEST_PATH));

    access instance(TEST_PATH);
    BOOST_REQUIRE(instance.destroy_());
}

BOOST_AUTO_TEST_CASE(file_lock__destroy__exists__true_deleted)
{
    BOOST_REQUIRE(test::create(TEST_PATH));

    access instance(TEST_PATH);
    BOOST_REQUIRE(instance.destroy_());
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_SUITE_END()
