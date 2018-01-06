/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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

#include <cstdint>
#include <bitcoin/database.hpp>
#include "../utility/test_map.hpp"

using namespace bc;
using namespace bc::database;

BOOST_AUTO_TEST_SUITE(hash_table_header_tests)

BOOST_AUTO_TEST_CASE(hash_table_header__create__always__sets_minimum_file_size)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE_EQUAL(file.size(), 0u);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_GE(file.size(), header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__start__default_file__success)
{
    test::test_map file;
    const auto buckets = 10u;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());
}

BOOST_AUTO_TEST_CASE(hash_table_header__start__undersized_file__failure)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(file.resize(header_size - 1));
    BOOST_REQUIRE(!header.start());
}

BOOST_AUTO_TEST_CASE(hash_table_header__start__oversized_file__success)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(file.resize(header_size + 1));
    BOOST_REQUIRE(header.start());
}

BOOST_AUTO_TEST_CASE(hash_table_header__buckets__default__expected)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE_EQUAL(header.buckets(), buckets);
}

BOOST_AUTO_TEST_CASE(hash_table_header__buckets__create__expected)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_EQUAL(header.buckets(), buckets);
}

BOOST_AUTO_TEST_CASE(hash_table_header__buckets__resize__expected)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(file.resize(header_size + 1));
    BOOST_REQUIRE_EQUAL(header.buckets(), buckets);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size1__32bit__expected)
{
    const auto buckets = 10u;
    const auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * buckets;
    const auto size = hash_table_header<uint32_t, uint32_t>::size(buckets);
    BOOST_REQUIRE_EQUAL(size, header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size1__8bit_and_64bit__expected)
{
    const auto buckets = 10u;
    const auto header_size = sizeof(uint8_t) + sizeof(uint64_t) * buckets;
    const auto size = hash_table_header<uint8_t, uint64_t>::size(buckets);
    BOOST_REQUIRE_EQUAL(size, header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size2__default__expected)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint8_t) + sizeof(uint64_t) * buckets;
    hash_table_header<uint8_t, uint64_t> header(file, buckets);
    BOOST_REQUIRE_EQUAL(header.size(), header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size2__create__expected)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint8_t) + sizeof(uint64_t) * buckets;
    hash_table_header<uint8_t, uint64_t> header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_EQUAL(header.size(), header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__size2__resize__expected)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = sizeof(uint8_t) + sizeof(uint64_t) * buckets;
    hash_table_header<uint8_t, uint64_t> header(file, buckets);
    BOOST_REQUIRE(file.resize(header_size + 1));
    BOOST_REQUIRE_EQUAL(header.size(), header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__read_write__32_bit_value__success)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = hash_table_header<uint32_t, uint32_t>::size(buckets);
    hash_table_header<uint32_t, uint32_t> header(file, buckets);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.resize(header_size));
    BOOST_REQUIRE(header.create());
    header.write(9, 42);
    header.write(0, 24);
    BOOST_REQUIRE_EQUAL(header.read(9), 42u);
    BOOST_REQUIRE_EQUAL(header.read(0), 24u);
}

BOOST_AUTO_TEST_CASE(hash_table_header__read_write__64_bit_value__success)
{
    test::test_map file;
    const auto buckets = 10u;
    const auto header_size = hash_table_header<uint32_t, uint64_t>::size(buckets);
    hash_table_header<uint32_t, uint64_t> header(file, buckets);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.resize(header_size));
    BOOST_REQUIRE(header.create());
    header.write(9, 42);
    header.write(0, 24);
    BOOST_REQUIRE_EQUAL(header.read(9), 42u);
    BOOST_REQUIRE_EQUAL(header.read(0), 24u);
}

BOOST_AUTO_TEST_SUITE_END()

