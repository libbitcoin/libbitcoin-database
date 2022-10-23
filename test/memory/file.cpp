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

BOOST_AUTO_TEST_CASE(file__clear_path__empty__true)
{
    BOOST_REQUIRE(clear_path(TEST_DIRECTORY));
}

BOOST_AUTO_TEST_CASE(file__clear_path__exists__true_cleared)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(clear_path(TEST_DIRECTORY));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__create_file__missing__true)
{
    BOOST_REQUIRE(create_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__create_file__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(create_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__file_exists__missing__false)
{
    BOOST_REQUIRE(!file_exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__file_exists__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file_exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__remove_file__missing__false)
{
    BOOST_REQUIRE(!remove_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__remove_file__exists__true_removed)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(remove_file(TEST_PATH));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__rename_file__missing__false)
{
    BOOST_REQUIRE(!rename_file(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__rename_file__exists_to_self__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(rename_file(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__rename_file__exists__true)
{
    const std::string target = TEST_PATH + "_";
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(rename_file(TEST_PATH, target));
    BOOST_REQUIRE(test::exists(target));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(rename_file(target, TEST_PATH));
    BOOST_REQUIRE(!test::exists(target));
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file__open_file__missing__failure)
{
	BOOST_REQUIRE_EQUAL(open_file(TEST_PATH), -1);
}

BOOST_AUTO_TEST_CASE(file__close_file__opened__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = open_file(TEST_PATH);
	BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE(close_file(descriptor));
}

BOOST_AUTO_TEST_CASE(file__file_size__empty__zero)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = open_file(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file_size(descriptor), zero);
    BOOST_REQUIRE(close_file(descriptor));
}

BOOST_AUTO_TEST_CASE(file__file_size__non_empty__expected)
{
    const std::string text = "panopticon";
    std::ofstream file(TEST_PATH);
    BOOST_REQUIRE(file.good());
    file << text;
    file.close();
    const auto descriptor = open_file(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file_size(descriptor), text.length());
    BOOST_REQUIRE(close_file(descriptor));
}

BOOST_AUTO_TEST_CASE(file__page_size__always__non_zero)
{
    BOOST_REQUIRE_NE(page_size(), zero);
}

BOOST_AUTO_TEST_SUITE_END()
