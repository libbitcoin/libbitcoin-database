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

BOOST_AUTO_TEST_SUITE(manager_tests)

using namespace system;

using key0 = system::data_array<0>;
using key1 = system::data_array<1>;
using key2 = system::data_array<2>;

// slabs

BOOST_AUTO_TEST_CASE(manager__count__empty_slab__zero)
{
    test::chunk_storage file;
    const manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE(is_zero(instance.count()));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__count__non_empty_slab__expected)
{
    constexpr auto expected = 42u;
    data_chunk buffer(expected, 0xff);
    test::chunk_storage file(buffer);

    // Slab sizing is byte-based (arbitrary, links are file offsets).
    const manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), expected);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__truncate__terminal_slab__false_unchanged)
{
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE(!instance.truncate(linkage<4>::terminal));
    BOOST_REQUIRE_EQUAL(instance.count(), zero);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__truncate__overflow_slab__false_unchanged)
{
    constexpr auto size = 42u;
    data_chunk buffer(size, 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE(!instance.truncate(add1(size)));
    BOOST_REQUIRE_EQUAL(instance.count(), size);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__truncate__half_full_slab__true_changed)
{
    constexpr auto size = 42u;
    constexpr auto half = to_half(size);
    data_chunk buffer(size, 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE(instance.truncate(half));
    BOOST_REQUIRE_EQUAL(instance.count(), half);

    // Can only truncate to logical limit.
    BOOST_REQUIRE(!instance.truncate(size));
    BOOST_REQUIRE_EQUAL(instance.count(), half);

    BOOST_REQUIRE(instance.truncate(0));
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__eof_slab__terminal_unchanged)
{
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<7>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(storage::eof), linkage<7>::terminal);
    BOOST_REQUIRE_EQUAL(instance.count(), zero);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__terminal_slab__terminal_unchanged)
{
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(linkage<4>::terminal), linkage<4>::terminal);
    BOOST_REQUIRE_EQUAL(instance.count(), zero);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__empty_slab__expected)
{
    constexpr auto expected = 42u;
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(expected), zero);
    BOOST_REQUIRE_EQUAL(instance.count(), expected);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__non_empty_slab__expected)
{
    constexpr auto expected = 42u;
    data_chunk buffer(to_half(expected), 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<4>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(to_half(expected)), to_half(expected));
    BOOST_REQUIRE_EQUAL(instance.count(), expected);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__get__terminal_slab__terminal)
{
    constexpr auto size = 14u;
    data_chunk buffer(size, 0xff);
    test::chunk_storage file(buffer);
    const manager<linkage<2>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), size);
    BOOST_REQUIRE(!instance.get(linkage<2>::terminal));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__get__slab__expected)
{
    constexpr auto size = 16u;
    data_chunk buffer
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    test::chunk_storage file(buffer);
    const manager<linkage<2>, key1, max_size_t> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), size);
    BOOST_REQUIRE_EQUAL(*instance.get(0)->begin(), 0x00_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(1)->begin(), 0x01_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(2)->begin(), 0x02_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(9)->begin(), 0x09_u8);
    BOOST_REQUIRE(!instance.get_fault());
}

// records

BOOST_AUTO_TEST_CASE(manager__count__empty_record__zero)
{
    test::chunk_storage file;
    const manager<linkage<4>, key1, 42> instance(file);
    BOOST_REQUIRE(is_zero(instance.count()));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__count__1_record__expected)
{
    constexpr auto count = 1u;
    constexpr auto bytes = 5u;
    constexpr auto expected = count * (sizeof(uint32_t) + bytes + array_count<key1>);
    data_chunk buffer(expected, 0xff);
    test::chunk_storage file(buffer);

    // Record sizing is record count-based (links are record counters).
    const manager<linkage<4>, key1, bytes> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__count__33_record__expected)
{
    constexpr auto count = 33u;
    constexpr auto bytes = 5u;
    constexpr auto expected = count * (sizeof(uint32_t) + bytes + array_count<key2>);
    data_chunk buffer(expected, 0xff);
    test::chunk_storage file(buffer);
    const manager<linkage<4>, key2, bytes> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), 33u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__truncate__terminal_record__false_unchanged)
{
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE(!instance.truncate(linkage<2>::terminal));
    BOOST_REQUIRE_EQUAL(instance.count(), zero);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__truncate__overflow_record__false_unchanged)
{
    data_chunk buffer(7, 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(!instance.truncate(2));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__truncate__half_full_record__true_changed)
{
    data_chunk buffer(14, 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE(instance.truncate(1));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    // Can only truncate to logical limit.
    BOOST_REQUIRE(!instance.truncate(2));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    BOOST_REQUIRE(instance.truncate(0));
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__terminal_empty_record__terminal_unchanged)
{
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(linkage<2>::terminal), linkage<2>::terminal);
    BOOST_REQUIRE_EQUAL(instance.count(), zero);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__terminal_non_empty_record__expected)
{
    data_chunk buffer(7, 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 1u);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.allocate(linkage<2>::terminal), linkage<2>::terminal);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__empty_record__expected)
{
    data_chunk buffer;
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 0u);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE_EQUAL(instance.allocate(2), 1u);
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__allocate__non_empty_record__expected)
{
    data_chunk buffer(7, 0xff);
    test::chunk_storage file(buffer);
    manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 1u);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.allocate(2), 2u);
    BOOST_REQUIRE_EQUAL(instance.count(), 4u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__get__terminal_record__terminal)
{
    data_chunk buffer(14, 0xff);
    test::chunk_storage file(buffer);
    const manager<linkage<2>, key0, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE(!instance.get(linkage<2>::terminal));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(manager__get__record__expected)
{
    data_chunk buffer
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    test::chunk_storage file(buffer);
    const manager<linkage<2>, key0, 6u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(*instance.get(0)->begin(), 0x00_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(1)->begin(), 0x06_u8);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_SUITE_END()
