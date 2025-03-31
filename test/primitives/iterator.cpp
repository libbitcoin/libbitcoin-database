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
#include "../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(iterator_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(iterator__self__empty__terminal)
{
    using link = linkage<4>;
    using key = data_array<0>;
    using slab_iterate = iterator<link, key, max_size_t>;
    constexpr auto start = link::terminal;
    constexpr key key0{};
    test::chunk_storage file;
    const slab_iterate iterator{ file.get(), start, key0 };
    BOOST_REQUIRE(iterator.get().is_terminal());
    BOOST_REQUIRE(!iterator);
    BOOST_REQUIRE_EQUAL(iterator.key(), key0);
}

BOOST_AUTO_TEST_CASE(iterator__self__overflow__terminal)
{
    using link = linkage<4>;
    using key = data_array<0>;
    using slab_iterate = iterator<link, key, max_size_t>;
    constexpr auto start = 13;
    constexpr key key0{};
    data_chunk data
    {
        0x00, 0x01, 0x02, 0x03,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    test::chunk_storage file{ data };
    const slab_iterate iterator{ file.get(), start, key0 };
    BOOST_REQUIRE(iterator->is_terminal());
    BOOST_REQUIRE(!iterator);
    BOOST_REQUIRE_EQUAL(iterator.key(), key0);
}

BOOST_AUTO_TEST_CASE(iterator__advance__record__expected)
{
    using link = linkage<1>;
    using key = data_array<2>;
    using record_iterate = iterator<link, key, 1>;
    constexpr auto start = 0;
    constexpr key key2{ 0x1a, 0x2a };
    data_chunk data
    {
        0x01, 0x1a, 0x2a, 0xee,
        0x02, 0x1a, 0x2a, 0xee,
        0xff, 0xcc, 0xcc, 0xee
    };
    test::chunk_storage file{ data };
    record_iterate iterator{ file.get(), start, key2 };
    BOOST_REQUIRE(iterator);
    BOOST_REQUIRE_EQUAL(iterator.get(), 0x00u);
    BOOST_REQUIRE(++iterator);
    BOOST_REQUIRE(iterator);
    BOOST_REQUIRE_EQUAL(iterator.get(), 0x01u);
    BOOST_REQUIRE(!iterator.advance());
    BOOST_REQUIRE(!iterator);
    BOOST_REQUIRE_EQUAL(*iterator, link::terminal);
    BOOST_REQUIRE_EQUAL(iterator.key(), key2);
}

BOOST_AUTO_TEST_CASE(iterator__advance__slab__expected)
{
    using link = linkage<1>;
    using key = data_array<2>;
    using slab_iterate = iterator<link, key, max_size_t>;

    constexpr auto start = 0;
    constexpr key key2{ 0x1a, 0x2a };
    data_chunk data
    {
        0x03, 0x1a, 0x2a,
        0x07, 0x1a, 0x2a, 0xee,
        0xff, 0xcc, 0xcc, 0xee, 0xee
    };
    test::chunk_storage file{ data };
    slab_iterate iterator{ file.get(), start, key2 };
    BOOST_REQUIRE(iterator);
    BOOST_REQUIRE_EQUAL(iterator.get(), 0x00u);
    BOOST_REQUIRE(iterator++);
    BOOST_REQUIRE(iterator);
    BOOST_REQUIRE_EQUAL(iterator.get(), 0x03u);
    BOOST_REQUIRE(!iterator.advance());
    BOOST_REQUIRE(!iterator);
    BOOST_REQUIRE_EQUAL(iterator.get(), link::terminal);
    BOOST_REQUIRE_EQUAL(iterator.key(), key2);

    iterator.reset();
    BOOST_REQUIRE_EQUAL(iterator.get(), link::terminal);
}

BOOST_AUTO_TEST_CASE(iterator__reset__always__sets_terminal_retains_key)
{
    using link = linkage<1>;
    using key = data_array<2>;
    using slab_iterate = iterator<link, key, max_size_t>;

    constexpr auto start = 0;
    constexpr key key2{ 0x1a, 0x2a };
    data_chunk data
    {
        0x03, 0x1a, 0x2a,
        0x07, 0x1a, 0x2a, 0xee,
        0xff, 0xcc, 0xcc, 0xee, 0xee
    };
    test::chunk_storage file{ data };
    slab_iterate iterator{ file.get(), start, key2 };
    BOOST_REQUIRE(iterator);
    BOOST_REQUIRE_EQUAL(*iterator, 0x00u);

    iterator.reset();
    BOOST_REQUIRE_EQUAL(*iterator, link::terminal);
    BOOST_REQUIRE_EQUAL(iterator.key(), key2);
}

BOOST_AUTO_TEST_SUITE_END()
