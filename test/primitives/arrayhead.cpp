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
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"
#include "../mocks/chunk_storage.hpp"

BC_AUTO_TEST_SUITE(arrayhead_tests)

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
using test_header = arrayhead<link, false>;

// TODO: see hashhead.
////class nullptr_storage
////  : public test::chunk_storages<one>
////{
////public:
////    using chunk_storages<one>::chunk_storages;
////
////    memory get(size_t size) const NOEXCEPT override
////    {
////        return is_zero(size) ? chunk_storages<one>::get(size) : memory{};
////    }
////};

BC_AUTO_TEST_CASE(arrayhead__create__size__expected)
{
    data_chunk data{};
    test::chunk_storage store{ data, test::terminal_fill };
    test_header head{ store, buckets };
    BC_REQUIRE(head.create());
    BC_REQUIRE_EQUAL(data.size(), head_size);
}

BC_AUTO_TEST_CASE(arrayhead__verify__uncreated__false)
{
    data_chunk data{};
    test::chunk_storage store{ data, test::terminal_fill };
    test_header head{ store, buckets };
    ////BC_REQUIRE(head.create());
    BC_REQUIRE(!head.verify());
}

BC_AUTO_TEST_CASE(arrayhead__verify__created__false)
{
    data_chunk data{};
    test::chunk_storage store{ data, test::terminal_fill };
    test_header head{ store, buckets };
    BC_REQUIRE(head.create());
    BC_REQUIRE(head.verify());
}

BC_AUTO_TEST_CASE(arrayhead__get_body_count__created__zero)
{
    data_chunk data{};
    test::chunk_storage store{ data, test::terminal_fill };
    test_header head{ store, buckets };
    BC_REQUIRE(head.create());

    link count{};
    BC_REQUIRE(head.get_body_count(count));
    BC_REQUIRE_EQUAL(count, zero);
}

BC_AUTO_TEST_CASE(arrayhead__set_body_count__get_body_count__expected)
{
    data_chunk data{};
    test::chunk_storage store{ data, test::terminal_fill };
    test_header head{ store, buckets };
    BC_REQUIRE(head.create());

    constexpr auto expected = 42u;
    BC_REQUIRE(head.set_body_count(expected));

    link count{};
    BC_REQUIRE(head.get_body_count(count));
    BC_REQUIRE_EQUAL(count, expected);
}

BC_AUTO_TEST_CASE(arrayhead__clear__get_body_count__zero)
{
    data_chunk data{};
    test::chunk_storage store{ data, test::terminal_fill };
    test_header head{ store, buckets };
    BC_REQUIRE(head.create());

    constexpr auto expected = 42u;
    BC_REQUIRE(head.set_body_count(expected));

    link count{};
    BC_REQUIRE(head.get_body_count(count));
    BC_REQUIRE_EQUAL(count, expected);

    BC_REQUIRE(head.clear());
    BC_REQUIRE(head.get_body_count(count));
    BC_REQUIRE_EQUAL(count, zero);
}

BC_AUTO_TEST_CASE(arrayhead__at__not_past_end__terminal)
{
    test::chunk_storage store{ test::terminal_fill };
    test_header head{ store, buckets };

    // create() allocates and fills buckets with terminal.
    BC_REQUIRE(head.create());
    BC_REQUIRE(head.at(zero).is_terminal());
}

BC_AUTO_TEST_CASE(arrayhead__at__past_end__terminal)
{
    test::chunk_storage store{ test::terminal_fill };
    test_header head{ store, buckets };

    // create() allocates and fills buckets with terminal.
    BC_REQUIRE(head.create());
    BC_REQUIRE(head.at(buckets).is_terminal());
}

BC_AUTO_TEST_SUITE_END()
