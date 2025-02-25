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

BOOST_AUTO_TEST_SUITE(arrayhead_tests)

using namespace system;

constexpr auto link_size = 5_size;
constexpr auto head_size = 105_size;

// Key size does not factor into head byte size (for search key only).
constexpr auto links = head_size / link_size;
static_assert(links == 21u);

// Bucket count is one less than link count, due to head.size field.
constexpr auto buckets = sub1(links);
static_assert(buckets == 20u);

using link = linkage<link_size>;
using test_header = arrayhead<link>;

class nullptr_storage
  : public test::chunk_storage
{
public:
    using chunk_storage::chunk_storage;

    memory_ptr get(size_t size) const NOEXCEPT override
    {
        return is_zero(size) ? chunk_storage::get(size) : nullptr;
    }
};

BOOST_AUTO_TEST_CASE(arrayhead__create__size__expected)
{
    data_chunk data;
    test::chunk_storage store{ data };
    test_header head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE_EQUAL(data.size(), head_size);
}

BOOST_AUTO_TEST_CASE(arrayhead__verify__uncreated__false)
{
    data_chunk data;
    test::chunk_storage store{ data };
    test_header head{ store, buckets };
    ////BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(!head.verify());
}

BOOST_AUTO_TEST_CASE(arrayhead__verify__created__false)
{
    data_chunk data;
    test::chunk_storage store{ data };
    test_header head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.verify());
}

BOOST_AUTO_TEST_CASE(arrayhead__get_body_count__created__zero)
{
    data_chunk data;
    test::chunk_storage store{ data };
    test_header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    link count{};
    BOOST_REQUIRE(head.get_body_count(count));
    BOOST_REQUIRE_EQUAL(count, zero);
}

BOOST_AUTO_TEST_CASE(arrayhead__set_body_count__get_body_count__expected)
{
    data_chunk data;
    test::chunk_storage store{ data };
    test_header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 42u;
    BOOST_REQUIRE(head.set_body_count(expected));

    link count{};
    BOOST_REQUIRE(head.get_body_count(count));
    BOOST_REQUIRE_EQUAL(count, expected);
}

BOOST_AUTO_TEST_CASE(arrayhead__clear__get_body_count__zero)
{
    data_chunk data;
    test::chunk_storage store{ data };
    test_header head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 42u;
    BOOST_REQUIRE(head.set_body_count(expected));

    link count{};
    BOOST_REQUIRE(head.get_body_count(count));
    BOOST_REQUIRE_EQUAL(count, expected);

    BOOST_REQUIRE(head.clear());
    BOOST_REQUIRE(head.get_body_count(count));
    BOOST_REQUIRE_EQUAL(count, zero);
}

BOOST_AUTO_TEST_CASE(arrayhead__at__key__terminal)
{
    test::chunk_storage store;
    test_header head{ store, buckets };

    // create() allocates and fills buckets with terminal.
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.at(zero).is_terminal());
}

BOOST_AUTO_TEST_SUITE_END()
