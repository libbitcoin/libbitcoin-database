/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

BC_FIXTURE_TEST_SUITE(file_utilities_tests, test::directory_setup_fixture)

using namespace system;
static_assert(file::invalid == -1);

// is_directory

BC_AUTO_TEST_CASE(file_utilities__is_directory__missing__false)
{
    BC_REQUIRE(!file::is_directory(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__is_directory__exists__true)
{
    BC_REQUIRE(file::is_directory(TEST_DIRECTORY));
}

// clear_directory

BC_AUTO_TEST_CASE(file_utilities__clear_directory__empty__true)
{
    BC_REQUIRE(file::clear_directory(TEST_DIRECTORY));
}

BC_AUTO_TEST_CASE(file_utilities__clear_directory__exists__true_cleared)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(test::exists(TEST_PATH));
    BC_REQUIRE(file::clear_directory(TEST_DIRECTORY));
    BC_REQUIRE(!test::exists(TEST_PATH));
}

BC_AUTO_TEST_CASE(clear_directory__missing__true)
{
    BC_REQUIRE(file::clear_directory(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__clear_directory__exists__true)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::clear_directory(TEST_PATH));
}

// create_directory

BC_AUTO_TEST_CASE(create_directory__missing__true)
{
    BC_REQUIRE(file::create_directory(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__create_directory__exists__false)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(!file::create_directory(TEST_PATH));
}

// is_file

BC_AUTO_TEST_CASE(file_utilities__is_file__missing__false)
{
    BC_REQUIRE(!file::is_file(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__is_file__exists__true)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::is_file(TEST_PATH));
}

// create_file

BC_AUTO_TEST_CASE(file_utilities__create_file__missing__true)
{
    BC_REQUIRE(file::create_file(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__create_file__exists__true)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::create_file(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__create_file__empty__created)
{
    const data_chunk source(0);
    BC_REQUIRE(!test::exists(TEST_PATH));
    BC_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BC_REQUIRE(test::exists(TEST_PATH));

    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, -1);

    size_t out{};
    BC_REQUIRE(file::size(out, descriptor));
    BC_REQUIRE_EQUAL(out, source.size());
    BC_REQUIRE(file::close(descriptor));
}

BC_AUTO_TEST_CASE(file_utilities__create_file__missing__expected_size)
{
    const data_chunk source(42u);
    BC_REQUIRE(!test::exists(TEST_PATH));
    BC_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BC_REQUIRE(test::exists(TEST_PATH));

    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, -1);

    size_t out{};
    BC_REQUIRE(file::size(out, descriptor));
    BC_REQUIRE_EQUAL(out, source.size());
    BC_REQUIRE(file::close(descriptor));
}

BC_AUTO_TEST_CASE(file_utilities__create_file__exists__replaced)
{
    const data_chunk old(100);
    BC_REQUIRE(!test::exists(TEST_PATH));
    BC_REQUIRE(file::create_file(TEST_PATH, old.data(), old.size()));
    BC_REQUIRE(test::exists(TEST_PATH));

    auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, -1);

    size_t out{};
    BC_REQUIRE(file::size(out, descriptor));
    BC_REQUIRE_EQUAL(out, old.size());
    BC_REQUIRE(file::close(descriptor));

    const data_chunk source(42);
    BC_REQUIRE(test::exists(TEST_PATH));
    BC_REQUIRE(file::create_file(TEST_PATH, source.data(), source.size()));
    BC_REQUIRE(test::exists(TEST_PATH));

    descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, -1);

    BC_REQUIRE(file::size(out, descriptor));
    BC_REQUIRE_EQUAL(out, source.size());
    BC_REQUIRE(file::close(descriptor));
}

// remove

BC_AUTO_TEST_CASE(file_utilities__remove__missing__true)
{
    BC_REQUIRE(file::remove(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__remove__exists__true_removed)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::remove(TEST_PATH));
    BC_REQUIRE(!test::exists(TEST_PATH));
}

// rename

BC_AUTO_TEST_CASE(file_utilities__rename__missing__false)
{
    BC_REQUIRE(!file::rename(TEST_PATH, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__rename__exists_to_self__true)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::rename(TEST_PATH, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__rename__exists__true)
{
    const std::string target = TEST_PATH + "_";
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::rename(TEST_PATH, target));
    BC_REQUIRE(test::exists(target));
    BC_REQUIRE(!test::exists(TEST_PATH));
    BC_REQUIRE(file::rename(target, TEST_PATH));
    BC_REQUIRE(!test::exists(target));
    BC_REQUIRE(test::exists(TEST_PATH));
}

// copy

BC_AUTO_TEST_CASE(file_utilities__copy__missing__false)
{
    BC_REQUIRE(!file::copy(TEST_PATH, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__copy__exists_to_self__false)
{
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(!file::copy(TEST_PATH, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__copy__target_missing__true_both_exist)
{
    const std::string target = TEST_PATH + "_";
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::copy(TEST_PATH, target));
    BC_REQUIRE(test::exists(target));
    BC_REQUIRE(test::exists(TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__copy__target_exists__false_both_exist)
{
    const std::string target = TEST_PATH + "_";
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(test::create(target));
    BC_REQUIRE(!file::copy(target, TEST_PATH));
    BC_REQUIRE(test::exists(target));
    BC_REQUIRE(test::exists(TEST_PATH));
}

// copy_directory

BC_AUTO_TEST_CASE(file_utilities__copy_directory__missing__false)
{
    BC_REQUIRE(!file::copy_directory(TEST_PATH, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__copy_directory__exists_to_self__false)
{
    BC_REQUIRE(file::create_directory(TEST_PATH));
    BC_REQUIRE(!file::copy_directory(TEST_PATH, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__copy_directory__target_exists__false)
{
    const std::string from_dir = TEST_PATH + "_from";
    const std::string to_dir = TEST_PATH + "_to";
    BC_REQUIRE(file::create_directory(from_dir));
    BC_REQUIRE(file::create_directory(to_dir));
    BC_REQUIRE(!file::copy_directory(from_dir, to_dir));
}

BC_AUTO_TEST_CASE(file_utilities__copy_directory__target_missing__true_copied)
{
    const std::string from_dir = TEST_PATH + "_from";
    const std::string to_dir = TEST_PATH + "_to";
    BC_REQUIRE(file::create_directory(from_dir));
    BC_REQUIRE(file::copy_directory(from_dir, to_dir));
    BC_REQUIRE(file::is_directory(to_dir));
}

BC_AUTO_TEST_CASE(file_utilities__copy_directory__target_missing__true_files_copied)
{
    const std::string from_dir = TEST_PATH + "_from";
    const std::string to_dir = TEST_PATH + "_to";
    const std::string from_file = from_dir + "/file";
    const std::string to_file = to_dir + "/file";
    BC_REQUIRE(file::create_directory(from_dir));
    BC_REQUIRE(file::create_file(from_file));
    BC_REQUIRE(file::copy_directory(from_dir, to_dir));
    BC_REQUIRE(file::is_directory(to_dir));
    BC_REQUIRE(file::is_file(to_file));
}

// open

BC_AUTO_TEST_CASE(file_utilities__open__missing__failure)
{
    BC_REQUIRE_EQUAL(file::open(TEST_PATH), -1);
}

// open_ex

BC_AUTO_TEST_CASE(file_utilities__open_ex__missing__failure)
{
    int file_descriptor{};
    BC_REQUIRE(file::open_ex(file_descriptor, TEST_PATH));
    BC_REQUIRE_EQUAL(file_descriptor, -1);
}

// close

BC_AUTO_TEST_CASE(file_utilities__close__opened__true)
{
    BC_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, file::invalid);
    BC_REQUIRE(file::close(descriptor));
}

// close_ex

BC_AUTO_TEST_CASE(file_utilities__close_ex__opened__success)
{
    BC_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, file::invalid);
    BC_REQUIRE(file::close(descriptor));
}

// size1

BC_AUTO_TEST_CASE(file_utilities__size1__invalid_handle__false)
{
    size_t out{};
    BC_REQUIRE(!file::size(out, -1));
}

BC_AUTO_TEST_CASE(file_utilities__size1__empty__true_zero)
{
    BC_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, -1);

    size_t out{ 42 };
    BC_REQUIRE(file::size(out, descriptor));
    BC_REQUIRE_EQUAL(out, zero);
    BC_REQUIRE(file::close(descriptor));
}

BC_AUTO_TEST_CASE(file_utilities__size1__non_empty__expected)
{
    const std::string text = "panopticon";
    BC_REQUIRE(test::create(TEST_PATH, text));

    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, file::invalid);

    size_t out{};
    BC_REQUIRE(file::size(out, descriptor));
    BC_REQUIRE_EQUAL(out, text.length());
    BC_REQUIRE(file::close(descriptor));
}

// size_ex1

BC_AUTO_TEST_CASE(file_utilities__size_ex1__invalid_handle__failure)
{
    size_t out{};
    BC_REQUIRE(file::size_ex(out, -1));
}

BC_AUTO_TEST_CASE(file_utilities__size_ex1__empty__success_zero)
{
    BC_REQUIRE(test::create(TEST_PATH));
    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, -1);

    size_t out{ 42 };
    BC_REQUIRE(!file::size_ex(out, descriptor));
    BC_REQUIRE_EQUAL(out, zero);
    BC_REQUIRE(file::close(descriptor));
}

BC_AUTO_TEST_CASE(file_utilities__size_ex1__non_empty__expected)
{
    const std::string text = "panopticon";
    BC_REQUIRE(test::create(TEST_PATH, text));

    const auto descriptor = file::open(TEST_PATH);
    BC_REQUIRE_NE(descriptor, file::invalid);

    size_t out{};
    BC_REQUIRE(!file::size_ex(out, descriptor));
    BC_REQUIRE_EQUAL(out, text.length());
    BC_REQUIRE(file::close(descriptor));
}

// size2

BC_AUTO_TEST_CASE(file_utilities__size2__missing__false)
{
    size_t out{};
    BC_REQUIRE(!file::size(out, TEST_PATH));
}

BC_AUTO_TEST_CASE(file_utilities__size2__empty__true_zero)
{
    size_t out{ 42 };
    BC_REQUIRE(test::create(TEST_PATH));
    BC_REQUIRE(file::size(out, TEST_PATH));
    BC_REQUIRE_EQUAL(out, zero);
}

BC_AUTO_TEST_CASE(file_utilities__size2__non_empty__true_expected)
{
    const std::string text = "panopticon";
    BC_REQUIRE(test::create(TEST_PATH, text));

    size_t out{};
    BC_REQUIRE(file::size(out, TEST_PATH));
    BC_REQUIRE_EQUAL(out, text.length());
}

// space

BC_AUTO_TEST_CASE(file_utilities__space__file__true)
{
    const std::string text = "panopticon";
    BC_REQUIRE(test::create(TEST_PATH, text));

    uint64_t out{};
    BC_REQUIRE(file::space(out, TEST_PATH));
    BC_WARN(is_nonzero(out));
}

BC_AUTO_TEST_CASE(file_utilities__space__create_directory__true)
{
    const std::filesystem::path directory = TEST_PATH + "/foo";
    BC_REQUIRE(file::create_directory(directory));

    uint64_t out{};
    BC_REQUIRE(file::space(out, directory));
    BC_WARN(is_nonzero(out));
}

BC_AUTO_TEST_SUITE_END()
