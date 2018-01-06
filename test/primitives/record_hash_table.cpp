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

#include <bitcoin/database.hpp>
#include "../utility/storage.hpp"
#include "../utility/utility.hpp"

using namespace bc;
using namespace bc::database;

BOOST_AUTO_TEST_SUITE(record_hash_table_tests)

// TODO: replace use of std::hash with fixed algorithms.
// These tests are sensitive to std::hash algorithm, which is opaque.
// This makes the tests unreliable and the store files non-portable.

#ifdef NOT_DEFINED

BOOST_AUTO_TEST_CASE(record_hash_table__32bit__test)
{
    typedef record_hash_table<test::tiny_hash> hash_table;

    const auto buckets = 2u;
    const auto value_size = 4u;
    const auto header_size = hash_table::header_type::size(buckets);
    const auto record_size = record_row<test::tiny_hash, hash_table::link_type>::size(value_size);

    test::storage file;
    BOOST_REQUIRE(file.open());

    hash_table::header_type header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_GE(file.size(), header_size);

    hash_table::record_manager manager(file, header_size, record_size);
    BOOST_REQUIRE(manager.create());
    BOOST_REQUIRE_GE(file.size(), header_size + sizeof(hash_table::link_type));

    record_hash_table<test::tiny_hash> table(header, manager);
    test::tiny_hash key{ { 0xde, 0xad, 0xbe, 0xef } };
    test::tiny_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b } };

    const auto write = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
    };

    const auto write1 = [](byte_serializer& serial)
    {
        serial.write_byte(99);
        serial.write_byte(98);
        serial.write_byte(97);
        serial.write_byte(96);
    };

    // [e][e]
    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
    BOOST_REQUIRE_EQUAL(header.read(1), header.empty);

    table.store(key, write);
    manager.sync();

    // [0][e]
    BOOST_REQUIRE_EQUAL(header.read(0), 0u);
    BOOST_REQUIRE_EQUAL(header.read(1), header.empty);

    table.store(key, write);
    manager.sync();

    // [1->0][e]
    BOOST_REQUIRE_EQUAL(header.read(0), 1u);

    table.store(key1, write1);
    manager.sync();

    // [1->0][2]
    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
    BOOST_REQUIRE_EQUAL(header.read(1), 2u);

    table.store(key1, write);
    manager.sync();

    // [1->0][3->2]
    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
    BOOST_REQUIRE_EQUAL(header.read(1), 3u);

    // Verify 0->empty
    record_row<test::tiny_hash, uint32_t> item0(manager, 0);
    BOOST_REQUIRE_EQUAL(item0.next_index(), header.empty);

    // Verify 1->0
    record_row<test::tiny_hash, uint32_t> item1(manager, 1);
    BOOST_REQUIRE_EQUAL(item1.next_index(), 0u);

    // Verify 2->empty
    record_row<test::tiny_hash, uint32_t> item2(manager, 2);
    BOOST_REQUIRE_EQUAL(item2.next_index(), header.empty);

    // Verify 3->2
    record_row<test::tiny_hash, uint32_t> item3(manager, 3);
    BOOST_REQUIRE_EQUAL(item3.next_index(), 2u);

    // [X->0][3->2]
    BOOST_REQUIRE(table.unlink(key));
    manager.sync();

    BOOST_REQUIRE_EQUAL(header.read(0), 0);
    BOOST_REQUIRE_EQUAL(header.read(1), 3u);

    // Verify 0->empty
    record_row<test::tiny_hash, uint32_t> item0a(manager, 0);
    BOOST_REQUIRE_EQUAL(item0a.next_index(), header.empty);

    // Verify 3->2
    record_row<test::tiny_hash, uint32_t> item3a(manager, 3);
    BOOST_REQUIRE_EQUAL(item3a.next_index(), 2u);

    // Verify 2->empty
    record_row<test::tiny_hash, uint32_t> item2a(manager, 2);
    BOOST_REQUIRE_EQUAL(item2a.next_index(), header.empty);

    // [0][X->2]
    BOOST_REQUIRE(table.unlink(key1));
    manager.sync();

    BOOST_REQUIRE_EQUAL(header.read(0), 0u);
    BOOST_REQUIRE_EQUAL(header.read(1), 2u);

    // Verify 0->empty
    record_row<test::tiny_hash, uint32_t> item0b(manager, 0);
    BOOST_REQUIRE_EQUAL(item0b.next_index(), header.empty);

    // Verify 2->empty
    record_row<test::tiny_hash, uint32_t> item2b(manager, 2);
    BOOST_REQUIRE_EQUAL(item2b.next_index(), header.empty);

    test::tiny_hash invalid{ { 0x00, 0x01, 0x02, 0x03 } };
    BOOST_REQUIRE(!table.unlink(invalid));
}

BOOST_AUTO_TEST_CASE(record_hash_table__64bit__test)
{
    typedef record_hash_table<test::little_hash> hash_table;

    const auto buckets = 2u;
    const auto value_size = 7u;
    const auto header_size = hash_table::header_type::size(buckets);
    const auto record_size = record_row<test::little_hash, hash_table::link_type>::size(value_size);

    test::storage file;
    BOOST_REQUIRE(file.open());

    hash_table::header_type header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE_GE(file.size(), header_size);

    hash_table::record_manager manager(file, header_size, record_size);
    BOOST_REQUIRE(manager.create());
    BOOST_REQUIRE_GE(file.size(), header_size + sizeof(hash_table::link_type));

    record_hash_table<test::little_hash> table(header, manager);

    test::little_hash key{ { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef } };
    test::little_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b } };

    const auto write = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
    };

    const auto write1 = [](byte_serializer& serial)
    {
        serial.write_byte(99);
        serial.write_byte(98);
        serial.write_byte(97);
        serial.write_byte(96);
        serial.write_byte(95);
        serial.write_byte(94);
        serial.write_byte(93);
    };

    table.store(key, write);
    manager.sync();

    // [e][0]
    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
    BOOST_REQUIRE_EQUAL(header.read(1), 0u);

    table.store(key, write);
    manager.sync();

    // [e][1->0]
    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
    BOOST_REQUIRE_EQUAL(header.read(1), 1u);

    table.store(key1, write1);
    manager.sync();

    // [2][1->0]
    BOOST_REQUIRE_EQUAL(header.read(0), 2u);
    BOOST_REQUIRE_EQUAL(header.read(1), 1u);

    table.store(key1, write);
    manager.sync();

    // [3->2][1->0]
    BOOST_REQUIRE_EQUAL(header.read(0), 3u);
    BOOST_REQUIRE_EQUAL(header.read(1), 1u);

    record_row<test::little_hash, uint32_t> item(manager, 3);
    BOOST_REQUIRE_EQUAL(item.next_index(), 2u);

    record_row<test::little_hash, uint32_t> item1(manager, 2);
    BOOST_REQUIRE_EQUAL(item1.next_index(), header.empty);

    // [3->2][X->0]
    BOOST_REQUIRE(table.unlink(key));
    manager.sync();

    BOOST_REQUIRE_EQUAL(header.read(1), 0u);

    // [X->2][X->0]
    BOOST_REQUIRE(table.unlink(key1));
    manager.sync();

    BOOST_REQUIRE_EQUAL(header.read(0), 2u);

    test::little_hash invalid{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } };
    BOOST_REQUIRE(!table.unlink(invalid));
}

#endif

BOOST_AUTO_TEST_SUITE_END()
