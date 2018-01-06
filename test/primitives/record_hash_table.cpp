/////**
//// * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#include <boost/test/unit_test.hpp>
////
////#include <bitcoin/database.hpp>
////#include "../utility/utility.hpp"
////
////using namespace bc;
////using namespace bc::database;
////
////BC_CONSTEXPR size_t buckets = 100;
////BC_CONSTEXPR size_t tx_size = 200;
////BC_CONSTEXPR size_t total_txs = 200;
////#define DIRECTORY "record_hash_table"
////
////struct record_hash_table_directory_setup_fixture
////{
////    record_hash_table_directory_setup_fixture()
////    {
////        BOOST_REQUIRE(test::clear_path(DIRECTORY));
////    }
////};
////
////BOOST_FIXTURE_TEST_SUITE(record_hash_table_tests, record_hash_table_directory_setup_fixture)
////
////BOOST_AUTO_TEST_CASE(record_hash_table__32bit__test)
////{
////    BC_CONSTEXPR size_t record_buckets = 2;
////    BC_CONSTEXPR size_t header_size = record_hash_table_header_size(record_buckets);
////    const auto path = DIRECTORY "/record_hash_table__32bit";
////
////    test::create(path);
////    file_map file(path);
////    BOOST_REQUIRE(file.open());
////
////    // Cannot hold an address reference because of following resize operation.
////    BOOST_REQUIRE(file.access()->buffer() != nullptr);
////
////    record_hash_table_header header(file, record_buckets);
////    BOOST_REQUIRE(header.create());
////    BOOST_REQUIRE(header.start());
////
////    typedef byte_array<4> tiny_hash;
////    BC_CONSTEXPR size_t record_size = hash_table_record_size<tiny_hash>(4);
////    const file_offset records_start = header_size;
////
////    record_manager alloc(file, records_start, record_size);
////    BOOST_REQUIRE(alloc.create());
////    BOOST_REQUIRE(alloc.start());
////
////    record_hash_table<tiny_hash> table(header, alloc);
////    tiny_hash key{ { 0xde, 0xad, 0xbe, 0xef } };
////    tiny_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b } };
////
////    const auto write = [](byte_serializer& serial)
////    {
////        serial.write_byte(110);
////        serial.write_byte(110);
////        serial.write_byte(4);
////        serial.write_byte(88);
////    };
////
////    const auto write1 = [](byte_serializer& serial)
////    {
////        serial.write_byte(99);
////        serial.write_byte(98);
////        serial.write_byte(97);
////        serial.write_byte(96);
////    };
////
////    // [e][e]
////    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
////    BOOST_REQUIRE_EQUAL(header.read(1), header.empty);
////
////    table.store(key, write);
////    alloc.sync();
////
////    // [0][e]
////    BOOST_REQUIRE_EQUAL(header.read(0), 0u);
////    BOOST_REQUIRE_EQUAL(header.read(1), header.empty);
////
////    table.store(key, write);
////    alloc.sync();
////
////    // [1->0][e]
////    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
////
////    table.store(key1, write1);
////    alloc.sync();
////
////    // [1->0][2]
////    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
////    BOOST_REQUIRE_EQUAL(header.read(1), 2u);
////
////    table.store(key1, write);
////    alloc.sync();
////
////    // [1->0][3->2]
////    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
////    BOOST_REQUIRE_EQUAL(header.read(1), 3u);
////
////    // Verify 0->empty
////    record_row<tiny_hash> item0(alloc, 0);
////    BOOST_REQUIRE_EQUAL(item0.next_index(), header.empty);
////
////    // Verify 1->0
////    record_row<tiny_hash> item1(alloc, 1);
////    BOOST_REQUIRE_EQUAL(item1.next_index(), 0u);
////
////    // Verify 2->empty
////    record_row<tiny_hash> item2(alloc, 2);
////    BOOST_REQUIRE_EQUAL(item2.next_index(), header.empty);
////
////    // Verify 3->2
////    record_row<tiny_hash> item3(alloc, 3);
////    BOOST_REQUIRE_EQUAL(item3.next_index(), 2u);
////
////    // [X->0][3->2]
////    BOOST_REQUIRE(table.unlink(key));
////    alloc.sync();
////
////    BOOST_REQUIRE_EQUAL(header.read(0), 0);
////    BOOST_REQUIRE_EQUAL(header.read(1), 3u);
////
////    // Verify 0->empty
////    record_row<tiny_hash> item0a(alloc, 0);
////    BOOST_REQUIRE_EQUAL(item0a.next_index(), header.empty);
////
////    // Verify 3->2
////    record_row<tiny_hash> item3a(alloc, 3);
////    BOOST_REQUIRE_EQUAL(item3a.next_index(), 2u);
////
////    // Verify 2->empty
////    record_row<tiny_hash> item2a(alloc, 2);
////    BOOST_REQUIRE_EQUAL(item2a.next_index(), header.empty);
////
////    // [0][X->2]
////    BOOST_REQUIRE(table.unlink(key1));
////    alloc.sync();
////
////    BOOST_REQUIRE_EQUAL(header.read(0), 0u);
////    BOOST_REQUIRE_EQUAL(header.read(1), 2u);
////
////    // Verify 0->empty
////    record_row<tiny_hash> item0b(alloc, 0);
////    BOOST_REQUIRE_EQUAL(item0b.next_index(), header.empty);
////
////    // Verify 2->empty
////    record_row<tiny_hash> item2b(alloc, 2);
////    BOOST_REQUIRE_EQUAL(item2b.next_index(), header.empty);
////
////    tiny_hash invalid{ { 0x00, 0x01, 0x02, 0x03 } };
////    BOOST_REQUIRE(!table.unlink(invalid));
////}
////
////BOOST_AUTO_TEST_CASE(record_hash_table__64bit__test)
////{
////    BC_CONSTEXPR size_t record_buckets = 2;
////    BC_CONSTEXPR size_t header_size = record_hash_table_header_size(record_buckets);
////
////    test::create(DIRECTORY "/record_hash_table_64bit");
////    file_map file(DIRECTORY "/record_hash_table_64bit");
////    BOOST_REQUIRE(file.open());
////
////    // Cannot hold an address reference because of following resize operation.
////    BOOST_REQUIRE(file.access()->buffer() != nullptr);
////
////    record_hash_table_header header(file, record_buckets);
////    BOOST_REQUIRE(header.create());
////    BOOST_REQUIRE(header.start());
////
////    typedef byte_array<8> little_hash;
////    BC_CONSTEXPR size_t record_size = hash_table_record_size<little_hash>(8);
////    const file_offset records_start = header_size;
////
////    record_manager alloc(file, records_start, record_size);
////    BOOST_REQUIRE(alloc.create());
////    BOOST_REQUIRE(alloc.start());
////
////    record_hash_table<little_hash> table(header, alloc);
////
////    little_hash key{ { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef } };
////    little_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b } };
////
////    const auto write = [](byte_serializer& serial)
////    {
////        serial.write_byte(110);
////        serial.write_byte(110);
////        serial.write_byte(4);
////        serial.write_byte(88);
////        serial.write_byte(110);
////        serial.write_byte(110);
////        serial.write_byte(4);
////        serial.write_byte(88);
////    };
////
////    const auto write1 = [](byte_serializer& serial)
////    {
////        serial.write_byte(99);
////        serial.write_byte(98);
////        serial.write_byte(97);
////        serial.write_byte(96);
////        serial.write_byte(95);
////        serial.write_byte(94);
////        serial.write_byte(93);
////        serial.write_byte(92);
////    };
////
////    table.store(key, write);
////    alloc.sync();
////
////    // [e][0]
////    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
////    BOOST_REQUIRE_EQUAL(header.read(1), 0u);
////
////    table.store(key, write);
////    alloc.sync();
////
////    // [e][1->0]
////    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
////    BOOST_REQUIRE_EQUAL(header.read(1), 1u);
////
////    table.store(key1, write1);
////    alloc.sync();
////
////    // [2][1->0]
////    BOOST_REQUIRE_EQUAL(header.read(0), 2u);
////    BOOST_REQUIRE_EQUAL(header.read(1), 1u);
////
////    table.store(key1, write);
////    alloc.sync();
////
////    // [3->2][1->0]
////    BOOST_REQUIRE_EQUAL(header.read(0), 3u);
////    BOOST_REQUIRE_EQUAL(header.read(1), 1u);
////
////    record_row<little_hash> item(alloc, 3);
////    BOOST_REQUIRE_EQUAL(item.next_index(), 2u);
////
////    record_row<little_hash> item1(alloc, 2);
////    BOOST_REQUIRE_EQUAL(item1.next_index(), header.empty);
////
////    // [3->2][X->0]
////    BOOST_REQUIRE(table.unlink(key));
////    alloc.sync();
////
////    BOOST_REQUIRE_EQUAL(header.read(1), 0u);
////
////    // [X->2][X->0]
////    BOOST_REQUIRE(table.unlink(key1));
////    alloc.sync();
////
////    BOOST_REQUIRE_EQUAL(header.read(0), 2u);
////
////    little_hash invalid{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } };
////    BOOST_REQUIRE(!table.unlink(invalid));
////}
////
////BOOST_AUTO_TEST_SUITE_END()
