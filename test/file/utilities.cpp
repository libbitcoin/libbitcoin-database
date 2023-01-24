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
 * You should have received a create_file of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"

struct utilities_setup_fixture
{
    DELETE_COPY_MOVE(utilities_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

        utilities_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~utilities_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(utilities_tests, utilities_setup_fixture)

using namespace system;
static_assert(file::invalid == -1);

BOOST_AUTO_TEST_CASE(utilities__is_directory__missing__false)
{
    BOOST_REQUIRE(!file::is_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__is_directory__exists__true)
{
    BOOST_REQUIRE(file::is_directory(TEST_DIRECTORY));
}

BOOST_AUTO_TEST_CASE(utilities__clear_directory__empty__true)
{
    BOOST_REQUIRE(file::clear_directory(TEST_DIRECTORY));
}

BOOST_AUTO_TEST_CASE(utilities__clear_directory__exists__true_cleared)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(file::clear_directory(TEST_DIRECTORY));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(clear_directory__missing__true)
{
    BOOST_REQUIRE(file::clear_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__clear_directory__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::clear_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(create_directory__missing__true)
{
    BOOST_REQUIRE(file::create_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__create_directory__exists__talse)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(!file::create_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__is_file__missing__false)
{
    BOOST_REQUIRE(!file::is_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__is_file__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::is_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__create_file__missing__true)
{
    BOOST_REQUIRE(file::create_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__create_file__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__create_file__empty__created)
{
    const data_chunk source(0);
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), source.size());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(utilities__create_file__missing__expected_size)
{
    const data_chunk source(42u);
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), source.size());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(utilities__create_file__exists__replaced)
{
    const data_chunk old(100);
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, old.data(), old.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), old.size());
    BOOST_REQUIRE(file::close(descriptor));

    const data_chunk source(42);
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), source.size());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(utilities__remove__missing__true)
{
    BOOST_REQUIRE(file::remove(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__remove__exists__true_removed)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::remove(TEST_PATH));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__rename__missing__false)
{
    BOOST_REQUIRE(!file::rename(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__rename__exists_to_self__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::rename(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(utilities__rename__exists__true)
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

BOOST_AUTO_TEST_CASE(utilities__open__missing__failure)
{
    BOOST_REQUIRE_EQUAL(file::open(TEST_PATH), -1);
}

BOOST_AUTO_TEST_CASE(utilities__close__opened__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, file::invalid);
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(utilities__size__empty__zero)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);
    BOOST_REQUIRE_EQUAL(file::size(descriptor), zero);
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(utilities__size__non_empty__expected)
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

BOOST_AUTO_TEST_CASE(utilities__page__always__non_zero)
{
    BOOST_REQUIRE_NE(file::page(), zero);
}

BOOST_AUTO_TEST_SUITE_END()
