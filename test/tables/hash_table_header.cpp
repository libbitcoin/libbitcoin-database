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

BOOST_AUTO_TEST_CASE(hash_table_header__create__allocation__expected)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    // create() calls allocate, expanding header storage.
    BOOST_REQUIRE_EQUAL(data.size(), header_size);
}

BOOST_AUTO_TEST_CASE(hash_table_header__create__get_body_size__zero)
{
    data_chunk data;
    test::storage store{ data };
    header head{ store, buckets };

    // create() initializes body size to zero.
    BOOST_REQUIRE(head.create());

    link size{};
    BOOST_REQUIRE(head.get_body_size(size));
    BOOST_REQUIRE_EQUAL(size, zero);
}

BOOST_AUTO_TEST_CASE(hash_table_header__hash__null_key__expected)
{
    constexpr key null_key{};
    const auto expected = system::djb2_hash(null_key) % buckets;

    test::storage store;
    header head{ store, buckets };
    BOOST_REQUIRE_EQUAL(head.hash(null_key), expected);
}

BOOST_AUTO_TEST_CASE(hash_table_header__head__null_key__terminal)
{
    constexpr key null_key{};

    test::storage store;
    header head{ store, buckets };

    // create() allocates and fills buckets with terminal.
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.head(null_key).is_terminal());
}

BOOST_AUTO_TEST_SUITE_END()
