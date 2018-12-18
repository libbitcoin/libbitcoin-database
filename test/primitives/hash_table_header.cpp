/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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

#include <algorithm>
#include <cstdint>
#include <bitcoin/database.hpp>
#include "../utility/storage.hpp"

using namespace bc;
using namespace bc::database;
using namespace bc::system;

BOOST_AUTO_TEST_SUITE(hash_table_header_tests)

BOOST_AUTO_TEST_CASE(hash_table_header__size__always__expected)
{
    typedef uint32_t index_type;
    typedef uint64_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    test::storage file;
    const auto buckets = 42u;
    const auto expected = sizeof(index_type) + sizeof(link_type) * buckets;
    header_type header(file, buckets);
    BOOST_REQUIRE_EQUAL(header.size(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__create__always__sets_minimum_file_size)
{
    test::storage file;
    hash_table_header<uint32_t, uint32_t> header(file, 10u);

    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE_EQUAL(file.size(), 0u);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_GE(file.size(), header.size());
}

BOOST_AUTO_TEST_CASE(hash_table_header__create__always__sets_bucket_count)
{
    typedef uint32_t index_type;
    typedef uint64_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    test::storage file;
    const auto expected = 42u;
    header_type header(file, expected);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());

    auto deserial = make_unsafe_deserializer(file.access()->buffer());
    BOOST_REQUIRE_EQUAL(deserial.template read_little_endian<index_type>(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__create__always__fills_empty_buckets)
{
    typedef uint32_t index_type;
    typedef uint64_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    test::storage file;
    header_type header(file, 10u);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());

    const auto buffer = file.access()->buffer();
    const auto start = buffer + sizeof(index_type);
    const auto empty = [](uint8_t byte) { return byte == (uint8_t)header_type::empty; };
    BOOST_REQUIRE(std::all_of(start, buffer + header.size(), empty));
}

BOOST_AUTO_TEST_CASE(hash_table_header__start__default_file__success)
{
    test::storage file;
    hash_table_header<uint32_t, uint32_t> header(file, 10u);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());
}

BOOST_AUTO_TEST_CASE(hash_table_header__start__undersized_file__failure)
{
    test::storage file;
    hash_table_header<uint32_t, uint32_t> header(file, 10u);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(file.resize(header.size() - 1));
    BOOST_REQUIRE(!header.start());
}

BOOST_AUTO_TEST_CASE(hash_table_header__start__oversized_file__success)
{
    test::storage file;
    hash_table_header<uint32_t, uint32_t> header(file, 10u);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(file.resize(header.size() + 1));
    BOOST_REQUIRE(header.start());
}

BOOST_AUTO_TEST_CASE(hash_table_header__buckets__default__expected)
{
    test::storage file;
    const auto expected = 10u;
    hash_table_header<uint32_t, uint32_t> header(file, expected);
    BOOST_REQUIRE_EQUAL(header.buckets(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__buckets__create__expected)
{
    test::storage file;
    const auto expected = 10u;
    hash_table_header<uint32_t, uint32_t> header(file, expected);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_EQUAL(header.buckets(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__buckets__resize__expected)
{
    test::storage file;
    const auto expected = 10u;
    hash_table_header<uint32_t, uint32_t> header(file, expected);
    BOOST_REQUIRE(file.resize(header.size() + 1));
    BOOST_REQUIRE_EQUAL(header.buckets(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size1__32bit__expected)
{
    typedef uint32_t index_type;
    typedef uint32_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    const auto buckets = 10u;
    const auto expected = sizeof(index_type) + sizeof(link_type) * buckets;
    BOOST_REQUIRE_EQUAL(header_type::size(buckets), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size1__8bit_and_64bit__expected)
{
    typedef uint8_t index_type;
    typedef uint64_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    const auto buckets = 10u;
    const auto expected = sizeof(index_type) + sizeof(link_type) * buckets;
    BOOST_REQUIRE_EQUAL(header_type::size(buckets), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size2__default__expected)
{
    typedef uint8_t index_type;
    typedef uint64_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    test::storage file;
    const auto buckets = 10u;
    const auto expected = sizeof(index_type) + sizeof(link_type) * buckets;
    header_type header(file, buckets);
    BOOST_REQUIRE_EQUAL(header.size(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size2__create__expected)
{
    typedef uint32_t index_type;
    typedef uint32_t link_type;
    typedef hash_table_header<index_type, link_type> header_type;

    test::storage file;
    const auto buckets = 10u;
    const auto expected = sizeof(index_type) + sizeof(link_type) * buckets;
    header_type header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_EQUAL(header.size(), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__read_write__32_bit_value__success)
{
    test::storage file;
    hash_table_header<uint32_t, uint32_t> header(file, 10u);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.resize(header.size()));
    BOOST_REQUIRE(header.create());
    header.write(9, 42);
    header.write(0, 24);
    BOOST_REQUIRE_EQUAL(header.read(9), 42u);
    BOOST_REQUIRE_EQUAL(header.read(0), 24u);
}

BOOST_AUTO_TEST_CASE(hash_table_header__read_write__64_bit_value__success)
{
    test::storage file;
    hash_table_header<uint32_t, uint64_t> header(file, 10u);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.resize(header.size()));
    BOOST_REQUIRE(header.create());
    header.write(9, 42);
    header.write(0, 24);
    BOOST_REQUIRE_EQUAL(header.read(9), 42u);
    BOOST_REQUIRE_EQUAL(header.read(0), 24u);
}

BOOST_AUTO_TEST_SUITE_END()
