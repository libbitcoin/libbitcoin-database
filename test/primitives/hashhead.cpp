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

BOOST_AUTO_TEST_SUITE(hashhead_tests)

using namespace system;

constexpr auto key_size = 10_size;
constexpr auto link_size = 5_size;
constexpr auto buckets = power2(4_size);
constexpr auto head_size = add1(buckets) * link_size;

// TODO: test at alignment (4 or 8 bytes).
constexpr auto cell_size = 5_size;

// Key size does not factor into head byte size (for search key only).
constexpr auto links = head_size / link_size;
static_assert(links == 17u);

// Bucket count is one less than link count, due to head.size field.
static_assert(buckets == 16u);

using link = linkage<link_size>;
using key = data_array<key_size>;
using hashhead_ = hashhead<link, key, cell_size>;

class nullptr_storage
  : public test::chunk_storages<one>
{
public:
    using chunk_storages<one>::chunk_storages;

    memory get(size_t size) const NOEXCEPT override
    {
        return is_zero(size) ? chunk_storages<one>::get(size) : memory{};
    }
};

BOOST_AUTO_TEST_CASE(hashhead__create__size__expected)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE_EQUAL(data.size(), head_size);
}

BOOST_AUTO_TEST_CASE(hashhead__verify__uncreated__false)
{
    data_chunk data{};;
    test::chunk_storage store{ data };
    hashhead_ head{ store, buckets };
    ////BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(!head.verify());
}

BOOST_AUTO_TEST_CASE(hashhead__verify__created__false)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.verify());
}

BOOST_AUTO_TEST_CASE(hashhead__get_body_count__created__zero)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());

    link count{};
    BOOST_REQUIRE(head.get_body_count(count));
    BOOST_REQUIRE_EQUAL(count, zero);
}

BOOST_AUTO_TEST_CASE(hashhead__set_body_count__get__expected)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 42u;
    BOOST_REQUIRE(head.set_body_count(expected));

    link count{};
    BOOST_REQUIRE(head.get_body_count(count));
    BOOST_REQUIRE_EQUAL(count, expected);
}

BOOST_AUTO_TEST_CASE(hashhead__unique_hash__null_key__expected)
{
    constexpr key null_key{};
    const auto expected = system::unique_hash(null_key) % buckets;
    BOOST_REQUIRE_EQUAL(expected, 0u);

    test::chunk_storage store;
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE_EQUAL(head.index(null_key), expected);
}

BOOST_AUTO_TEST_CASE(hashhead__key_hash__null_point__zero)
{
    const system::chain::point null_point{};
    BOOST_REQUIRE(is_zero(keys::bucket(null_point, 42u)));
}

BOOST_AUTO_TEST_CASE(hashhead__top__link__terminal)
{
    test::chunk_storage store;
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.top(9).is_terminal());
}

BOOST_AUTO_TEST_CASE(hashhead__top__nullptr__terminal)
{
    nullptr_storage store;
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.top(9).is_terminal());
}

BOOST_AUTO_TEST_CASE(hashhead__top__key__terminal)
{
    constexpr key null_key{};

    test::chunk_storage store;
    hashhead_ head{ store, buckets };

    // create() allocates and fills buckets with terminal.
    BOOST_REQUIRE(head.create());
    BOOST_REQUIRE(head.top(null_key).is_terminal());
}

BOOST_AUTO_TEST_CASE(hashhead__push__key__terminal)
{
    test::chunk_storage store;
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.create());

    constexpr auto expected = 2u;
    typename link::bytes next{ 42u };
    constexpr key null_key{};
    constexpr link current{ expected };
    head.push(current, next, null_key);

    // The terminal value at head[9|null_key] is copied to current.next.
    BOOST_REQUIRE(link{ next }.is_terminal());

    // The current link is copied to head[9|null_key].
    BOOST_REQUIRE_EQUAL(head.top(null_key), expected);
}

// filtered head (mirrors ins geometry: cell 8, link 4, m = 32, legacy k = 5)

using link4 = linkage<4>;
using filtered_ = hashhead<link4, key, 8>;
constexpr auto filtered_buckets = 16_size;

BOOST_AUTO_TEST_CASE(hashhead__optimal_k__zeros__legacy)
{
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(0, 100), 5u);
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(100, 0), 5u);
}

BOOST_AUTO_TEST_CASE(hashhead__optimal_k__load_factor__banded)
{
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(25, 10), 4u);
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(40, 10), 4u);
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(60, 10), 3u);
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(75, 10), 2u);
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(100, 10), 2u);
    BOOST_REQUIRE_EQUAL(filtered_::optimal_k(200, 10), 1u);
}

BOOST_AUTO_TEST_CASE(hashhead__construct__expected__derived_k)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    filtered_ head{ store, filtered_buckets, 40 };
    BOOST_REQUIRE_EQUAL(head.filter_k(), 4u);
}

BOOST_AUTO_TEST_CASE(hashhead__construct__no_expected__legacy_k)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    filtered_ head{ store, filtered_buckets };
    BOOST_REQUIRE_EQUAL(head.filter_k(), 5u);
}

BOOST_AUTO_TEST_CASE(hashhead__set_filter_k__valid__set)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    filtered_ head{ store, filtered_buckets };
    BOOST_REQUIRE(head.set_filter_k(2));
    BOOST_REQUIRE_EQUAL(head.filter_k(), 2u);
}

BOOST_AUTO_TEST_CASE(hashhead__set_filter_k__invalid__false)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    filtered_ head{ store, filtered_buckets };
    BOOST_REQUIRE(!head.set_filter_k(0));
    BOOST_REQUIRE(!head.set_filter_k(13));
    BOOST_REQUIRE_EQUAL(head.filter_k(), 5u);
}

BOOST_AUTO_TEST_CASE(hashhead__set_buckets__valid__set)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    filtered_ head{ store, filtered_buckets };
    BOOST_REQUIRE(head.set_buckets(42));
    BOOST_REQUIRE_EQUAL(head.buckets(), 42u);
    BOOST_REQUIRE_EQUAL(head.size(), 43u * 8u);
}

BOOST_AUTO_TEST_CASE(hashhead__set_buckets__over_terminal__false)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    filtered_ head{ store, filtered_buckets };
    BOOST_REQUIRE(!head.set_buckets(add1<size_t>(link4::terminal)));
    BOOST_REQUIRE_EQUAL(head.buckets(), filtered_buckets);
}

BOOST_AUTO_TEST_CASE(hashhead__set_filter_k__disabled__zero_only)
{
    data_chunk data{};
    test::chunk_storage store{ data };
    hashhead_ head{ store, buckets };
    BOOST_REQUIRE(head.set_filter_k(0));
    BOOST_REQUIRE(!head.set_filter_k(1));
    BOOST_REQUIRE_EQUAL(head.filter_k(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
