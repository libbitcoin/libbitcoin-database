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

struct file_utilities_setup_fixture
{
    DELETE_COPY_MOVE(file_utilities_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

        file_utilities_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~file_utilities_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(file_utilities_tests, file_utilities_setup_fixture)

using namespace system;
static_assert(file::invalid == -1);

BOOST_AUTO_TEST_CASE(file_utilities__is_directory__missing__false)
{
    BOOST_REQUIRE(!file::is_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__is_directory__exists__true)
{
    BOOST_REQUIRE(file::is_directory(TEST_DIRECTORY));
}

BOOST_AUTO_TEST_CASE(file_utilities__clear_directory__empty__true)
{
    BOOST_REQUIRE(file::clear_directory(TEST_DIRECTORY));
}

BOOST_AUTO_TEST_CASE(file_utilities__clear_directory__exists__true_cleared)
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

BOOST_AUTO_TEST_CASE(file_utilities__clear_directory__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::clear_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(create_directory__missing__true)
{
    BOOST_REQUIRE(file::create_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__create_directory__exists__talse)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(!file::create_directory(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__is_file__missing__false)
{
    BOOST_REQUIRE(!file::is_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__is_file__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::is_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__create_file__missing__true)
{
    BOOST_REQUIRE(file::create_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__create_file__exists__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__create_file__empty__created)
{
    const data_chunk source(0);
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);

    size_t out{};
    BOOST_REQUIRE(file::size(out, descriptor));
    BOOST_REQUIRE_EQUAL(out, source.size());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file_utilities__create_file__missing__expected_size)
{
    const data_chunk source(42u);
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);

    size_t out{};
    BOOST_REQUIRE(file::size(out, descriptor));
    BOOST_REQUIRE_EQUAL(out, source.size());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file_utilities__create_file__exists__replaced)
{
    const data_chunk old(100);
    BOOST_REQUIRE(!test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, old.data(), old.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);

    size_t out{};
    BOOST_REQUIRE(file::size(out, descriptor));
    BOOST_REQUIRE_EQUAL(out, old.size());
    BOOST_REQUIRE(file::close(descriptor));

    const data_chunk source(42);
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BOOST_REQUIRE(test::exists(TEST_PATH));

    descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);

    BOOST_REQUIRE(file::size(out, descriptor));
    BOOST_REQUIRE_EQUAL(out, source.size());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file_utilities__remove__missing__true)
{
    BOOST_REQUIRE(file::remove(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__remove__exists__true_removed)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::remove(TEST_PATH));
    BOOST_REQUIRE(!test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__rename__missing__false)
{
    BOOST_REQUIRE(!file::rename(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__rename__exists_to_self__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::rename(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__rename__exists__true)
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

BOOST_AUTO_TEST_CASE(file_utilities__copy__missing__false)
{
    BOOST_REQUIRE(!file::copy(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy__exists_to_self__false)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(!file::copy(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy__target_missing__true_both_exist)
{
    const std::string target = TEST_PATH + "_";
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::copy(TEST_PATH, target));
    BOOST_REQUIRE(test::exists(target));
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy__target_exists__false_both_exist)
{
    const std::string target = TEST_PATH + "_";
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(test::create(target));
    BOOST_REQUIRE(!file::copy(target, TEST_PATH));
    BOOST_REQUIRE(test::exists(target));
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy_directory__missing__false)
{
    BOOST_REQUIRE(!file::copy_directory(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy_directory__exists_to_self__false)
{
    BOOST_REQUIRE(file::create_directory(TEST_PATH));
    BOOST_REQUIRE(!file::copy_directory(TEST_PATH, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy_directory__target_exists__false)
{
    const std::string from_dir = TEST_PATH + "_from";
    const std::string to_dir = TEST_PATH + "_to";
    BOOST_REQUIRE(file::create_directory(from_dir));
    BOOST_REQUIRE(file::create_directory(to_dir));
    BOOST_REQUIRE(!file::copy_directory(from_dir, to_dir));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy_directory__target_missing__true_copied)
{
    const std::string from_dir = TEST_PATH + "_from";
    const std::string to_dir = TEST_PATH + "_to";
    BOOST_REQUIRE(file::create_directory(from_dir));
    BOOST_REQUIRE(file::copy_directory(from_dir, to_dir));
    BOOST_REQUIRE(file::is_directory(to_dir));
}

BOOST_AUTO_TEST_CASE(file_utilities__copy_directory__target_missing__true_files_copied)
{
    const std::string from_dir = TEST_PATH + "_from";
    const std::string to_dir = TEST_PATH + "_to";
    const std::string from_file = from_dir + "/file";
    const std::string to_file = to_dir + "/file";
    BOOST_REQUIRE(file::create_directory(from_dir));
    BOOST_REQUIRE(file::create_file(from_file));
    BOOST_REQUIRE(file::copy_directory(from_dir, to_dir));
    BOOST_REQUIRE(file::is_directory(to_dir));
    BOOST_REQUIRE(file::is_file(to_file));
}

BOOST_AUTO_TEST_CASE(file_utilities__open__missing__failure)
{
    BOOST_REQUIRE_EQUAL(file::open(TEST_PATH), -1);
}

BOOST_AUTO_TEST_CASE(file_utilities__close__opened__true)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, file::invalid);
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file_utilities__size1__invalid_handle__false)
{
    size_t out{};
    BOOST_REQUIRE(!file::size(out, -1));
}

BOOST_AUTO_TEST_CASE(file_utilities__size1__empty__true_zero)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, -1);

    size_t out{ 42 };
    BOOST_REQUIRE(file::size(out, descriptor));
    BOOST_REQUIRE_EQUAL(out, zero);
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file_utilities__size1__non_empty__expected)
{
    const std::string text = "panopticon";
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    const auto descriptor = file::open(TEST_PATH);
    BOOST_REQUIRE_NE(descriptor, file::invalid);

    size_t out{};
    BOOST_REQUIRE(file::size(out, descriptor));
    BOOST_REQUIRE_EQUAL(out, text.length());
    BOOST_REQUIRE(file::close(descriptor));
}

BOOST_AUTO_TEST_CASE(file_utilities__size2__missing__false)
{
    size_t out{};
    BOOST_REQUIRE(!file::size(out, TEST_PATH));
}

BOOST_AUTO_TEST_CASE(file_utilities__size2__empty__true_zero)
{
    size_t out{ 42 };
    BOOST_REQUIRE(test::create(TEST_PATH));
    BOOST_REQUIRE(file::size(out, TEST_PATH));
    BOOST_REQUIRE_EQUAL(out, zero);
}

BOOST_AUTO_TEST_CASE(file_utilities__size2__non_empty__true_expected)
{
    const std::string text = "panopticon";
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    size_t out{};
    BOOST_REQUIRE(file::size(out, TEST_PATH));
    BOOST_REQUIRE_EQUAL(out, text.length());
}

BOOST_AUTO_TEST_CASE(file_utilities__space__file__true)
{
    const std::string text = "panopticon";
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    size_t out{};
    BOOST_REQUIRE(file::space(out, TEST_PATH));
    BOOST_WARN(is_nonzero(out));
}

BOOST_AUTO_TEST_CASE(file_utilities__space__create_directory__true)
{
    const std::filesystem::path directory = TEST_PATH + "/foo";
    BOOST_REQUIRE(file::create_directory(directory));

    size_t out{};
    BOOST_REQUIRE(file::space(out, directory));
    BOOST_WARN(is_nonzero(out));
}

BOOST_AUTO_TEST_SUITE_END()
