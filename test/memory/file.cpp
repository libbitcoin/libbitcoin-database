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

struct file_setup_fixture
{
    DELETE4(file_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    file_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~file_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(file_tests, file_setup_fixture)

static_assert(file::invalid == -1);

BOOST_AUTO_TEST_CASE(file__clear__empty__true)
{
    BOOST_REQUIRE(file::clear(TEST_DIRECTORY));
}

BOOST_AUTO_TEST_CASE(file__clear__exists__true_cleared)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(file::clear(TEST_DIRECTORY));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__create__missing__true)
{
    BOOST_REQUIRE(file::create(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__create__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::create(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__exists__missing__false)
{
    BOOST_REQUIRE(!file::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__exists__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__remove__missing__false)
{
    BOOST_REQUIRE(!file::remove(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__remove__exists__true_removed)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::remove(TEST_PATH));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__rename__missing__false)
{
    BOOST_REQUIRE(!file::rename(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__rename__exists_to_self__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::rename(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__rename__exists__true)
{
    const std::string target = TEST_PATH + "_";
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::rename(TEST_PATH, target));
    BOOST_REQUIRE(test::exists(target));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::rename(target, TEST_PATH));
    BOOST_REQUIRE(!test::exists(target));
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__open__missing__failure)
{
    BOOST_REQUIRE_EQUAL(file::open(TEST_PATH), -1);
}

BOOST_AUTO_TEST_CASE(file__close__opened__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, file::invalid);
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file__size__empty__zero)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), zero);
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file__size__non_empty__expected)
{
    const std::string text = "panopticon";
    system::ofstream file(TEST_PATH);
    BOOST_REQUIRE(file.good());
    file << text;
    file.close();
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, file::invalid);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), text.length());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file__page__always__non_zero)
{
    BOOST_REQUIRE_NE(file::page(), zero);
}

BOOST_AUTO_TEST_SUITE_END()
