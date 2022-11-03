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
#include "../storage.hpp"

BOOST_AUTO_TEST_SUITE(hash_table_header_tests)

constexpr auto key_size = 10_size;
constexpr auto link_size = 5_size;
constexpr auto header_size = 105_size;

// Key size does not factor into header byte size (for search key only).
constexpr auto links = header_size / link_size;
static_assert(links == 21u);

// Bucket count is one less than link count, due to header.size field.
constexpr auto buckets = sub1(links);
static_assert(buckets == 20u);

using link = linkage<link_size>;
using key = data_array<key_size>;
using header = hash_table_header<link, key>;

BOOST_AUTO_TEST_CASE(hash_table_header__create__size__expected)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE_EQUAL(data.size(), header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__verify__uncreated__false)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };
    ////BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(!head.verify());
}

BOOST_AUTO_TEST_CASE(hash_table_header__verify__created__false)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.verify());
}

BOOST_AUTO_TEST_CASE(hash_table_header__get_body_count__create__zero)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    link size{};
    BOOST_REQUIRE(head.get_body_count(size));
    BOOST_REQUIRE_EQUAL(size, zero);
}

BOOST_AUTO_TEST_CASE(hash_table_header__get_body_count__set_body_count__expected)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 42u;
    BOOST_REQUIRE(head.set_body_count(expected));

    link size{};
    BOOST_REQUIRE(head.get_body_count(size));
    BOOST_REQUIRE_EQUAL(size, expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__index__null_key__expected)
{
    constexpr key null_key{};
    const auto expected = system::djb2_hash(null_key) % buckets;
    BOOST_REQUIRE_EQUAL(expected, 9u);

    test::storage store;
    header head{ store, buckets };
    BOOST_REQUIRE_EQUAL(head.index(null_key), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__head__link__terminal)
{
    test::storage store;
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.head(9).is_terminal());
}

BOOST_AUTO_TEST_CASE(hash_table_header__head__key__terminal)
{
    constexpr key null_key{};

    test::storage store;
    header head{ store, buckets };

    // create() allocates and fills buckets with terminal.
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.head(null_key).is_terminal());
}

BOOST_AUTO_TEST_CASE(hash_table_header__push__link__terminal)
{
    test::storage store;
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 2u;
    link next{ 42u };
    constexpr link link_key{ 9u };
    constexpr link current{ expected };
    BOOST_REQUIRE(head.push(current, next, link_key));

    // The terminal value at header[9|null_key] is copied to current.next.
    BOOST_REQUIRE(next.is_terminal());

    // The current link is copied to header[9|null_key].
    BOOST_REQUIRE_EQUAL(head.head(link_key), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__push__key__terminal)
{
    test::storage store;
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 2u;
    link next{ 42u };
    constexpr key null_key{};
    constexpr link current{ expected };
    BOOST_REQUIRE(head.push(current, next, null_key));

    // The terminal value at header[9|null_key] is copied to current.next.
    BOOST_REQUIRE(next.is_terminal());

    // The current link is copied to header[9|null_key].
    BOOST_REQUIRE_EQUAL(head.head(null_key), expected);
}

BOOST_AUTO_TEST_SUITE_END()
