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

BOOST_AUTO_TEST_SUITE(hash_table_tests)

BOOST_AUTO_TEST_CASE(hash_table__slab__one_element__round_trips)
{
    // Define hash table type.
    typedef test::tiny_hash key_type;
    typedef uint32_t index_type;
    typedef uint32_t link_type;
    typedef hash_table<slab_manager<link_type>, index_type, link_type, key_type> slab_map;

    // Create the file and initialize hash table.
    test::storage file;
    BOOST_REQUIRE(file.open());
    slab_map table(file, 100u);
    BOOST_REQUIRE(table.create());

    // Not requried when creating.
    BOOST_REQUIRE(table.start());

    const key_type key{ { 0xde, 0xad, 0xbe, 0xef } };

    const auto writer = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(99);
    };

    // Allocate, create and store a new element.
    auto element = table.allocator();
    const auto link = element.create(key, writer, 3);
    table.link(element);

    const auto reader = [](byte_deserializer& deserial)
    {
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 110u);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 4u);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 99u);
    };

    // Find, read and verify the new element (by key).
    const auto const_element = table.find(key);
    BOOST_REQUIRE(const_element);
    BOOST_REQUIRE_EQUAL(const_element.link(), link);
    BOOST_REQUIRE_EQUAL(const_element.next(), slab_map::not_found);
    BOOST_REQUIRE(const_element.key() == key);
    BOOST_REQUIRE(const_element.match(key));
    const_element.read(reader);

    // Not required when discarding.
    table.commit();
}

BOOST_AUTO_TEST_CASE(hash_table__slab__multiple_elements__expected)
{
    // Define hash table type.
    typedef test::tiny_hash key_type;
    typedef uint32_t index_type;
    typedef uint64_t link_type;
    typedef hash_table<slab_manager<link_type>, index_type, link_type, key_type> slab_map;

    // Create the file and initialize hash table.
    test::storage file;
    BOOST_REQUIRE(file.open());
    slab_map table(file, 100u);
    BOOST_REQUIRE(table.create());

    const key_type key1{ { 0xde, 0xad, 0xbe, 0xef } };
    const key_type key2{ { 0xba, 0xad, 0xbe, 0xef } };

    const auto writer1 = [](byte_serializer& serial)
    {
        serial.write_byte(42);
        serial.write_byte(24);
    };

    const auto writer2 = [](byte_serializer& serial)
    {
        serial.write_byte(44);
    };

    // Allocate, create and store a new elements.
    auto element = table.allocator();
    const auto link1 = element.create(key1, writer1, 2);
    table.link(element);
    const auto link2 = element.create(key2, writer2, 1);
    table.link(element);

    const auto reader1 = [](byte_deserializer& deserial)
    {
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 42);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 24);
    };

    const auto reader2 = [](byte_deserializer& deserial)
    {
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 44);
    };

    // Find, read and verify the new elements (by links).
    const auto const_element1 = table.find(link1);
    BOOST_REQUIRE(const_element1);
    BOOST_REQUIRE_EQUAL(const_element1.link(), link1);
    BOOST_REQUIRE_EQUAL(const_element1.next(), slab_map::not_found);
    BOOST_REQUIRE(const_element1.match(key1));
    const_element1.read(reader1);

    const auto const_element2 = table.find(link2);
    BOOST_REQUIRE(const_element2);
    BOOST_REQUIRE_EQUAL(const_element2.link(), link2);
    BOOST_REQUIRE_EQUAL(const_element2.next(), slab_map::not_found);
    BOOST_REQUIRE(const_element2.match(key2));
    const_element2.read(reader2);
}

BOOST_AUTO_TEST_CASE(hash_table__record__unlink_first_stored__expected)
{
    // Define hash table type.
    typedef test::tiny_hash key_type;
    typedef uint32_t index_type;
    typedef uint64_t link_type;
    typedef hash_table<slab_manager<link_type>, index_type, link_type, key_type> slab_map;

    // Create the file and initialize hash table.
    test::storage file;
    BOOST_REQUIRE(file.open());
    slab_map table(file, 100u);
    BOOST_REQUIRE(table.create());

    const key_type key1{ { 0xde, 0xad, 0xbe, 0xef } };
    const key_type key2{ { 0xba, 0xad, 0xbe, 0xef } };

    const auto writer1 = [](byte_serializer& serial)
    {
        serial.write_byte(42);
        serial.write_byte(24);
    };

    const auto writer2 = [](byte_serializer& serial)
    {
        serial.write_byte(44);
    };

    // Allocate, create and store a new elements.
    auto element = table.allocator();
    /*const auto link1 =*/ element.create(key1, writer1, 2);
    table.link(element);
    const auto link2 = element.create(key2, writer2, 1);
    table.link(element);

    // Unlink first element.
    BOOST_REQUIRE(table.unlink(key1));
    BOOST_REQUIRE(!table.unlink(key1));

    const auto reader2 = [](byte_deserializer& deserial)
    {
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 44);
    };

    // Find, read and verify the new elements (by keys).
    const auto const_element1 = table.find(key1);
    BOOST_REQUIRE(!const_element1);

    const auto const_element2 = table.find(key2);
    BOOST_REQUIRE(const_element2);
    BOOST_REQUIRE_EQUAL(const_element2.link(), link2);
    BOOST_REQUIRE_EQUAL(const_element2.next(), slab_map::not_found);
    BOOST_REQUIRE(const_element2.match(key2));
    const_element2.read(reader2);
}

BOOST_AUTO_TEST_CASE(hash_table__record__multiple_elements_32_bit__round_trips)
{
    // Define hash table type.
    typedef test::tiny_hash key_type;
    typedef uint32_t index_type;
    typedef uint32_t link_type;
    typedef hash_table<record_manager<link_type>, index_type, link_type, key_type> record_map;

    test::storage file;
    BOOST_REQUIRE(file.open());
    record_map table(file, 2u, 4u);
    BOOST_REQUIRE(table.create());

    const key_type key1{ { 0xde, 0xad, 0xbe, 0xef } };
    const key_type key2{ { 0xb0, 0x0b, 0xb0, 0x0b } };
    const key_type invalid{ { 0x00, 0x01, 0x02, 0x03 } };

    const auto writer1 = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
    };

    const auto writer2 = [](byte_serializer& serial)
    {
        serial.write_byte(99);
        serial.write_byte(98);
        serial.write_byte(97);
        serial.write_byte(96);
    };

    // Allocate, create and store a new elements.
    auto element = table.allocator();
    /*const auto link1 =*/ element.create(key1, writer1);
    table.link(element);
    const auto link2 = element.create(key2, writer2);
    table.link(element);
    const auto link3 = element.create(key2, writer2);
    table.link(element);

    BOOST_REQUIRE(table.unlink(key1));
    BOOST_REQUIRE(table.unlink(key2));
    BOOST_REQUIRE(!table.unlink(invalid));

    const auto reader = [](byte_deserializer& deserial)
    {
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 99);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 98);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 97);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 96);
    };

    // Key1 has been unlinked.
    BOOST_REQUIRE(!table.find(key1));

    // Second key2 has been unlinked, but is still present in the store.
    BOOST_REQUIRE(table.find(link3));

    // There is only one instance of key2 (next is not_found).
    auto const_element = table.find(key2);
    BOOST_REQUIRE(const_element);
    BOOST_REQUIRE_EQUAL(const_element.link(), link2);
    BOOST_REQUIRE_EQUAL(const_element.next(), record_map::not_found);
    BOOST_REQUIRE(const_element.match(key2));
    const_element.read(reader);
}

BOOST_AUTO_TEST_CASE(hash_table__record__multiple_elements_64_bit__round_trips)
{
    // Define hash table type.
    typedef test::little_hash key_type;
    typedef uint64_t index_type;
    typedef uint64_t link_type;
    typedef hash_table<record_manager<link_type>, index_type, link_type, key_type> record_map;

    // Create the file and initialize hash table.
    test::storage file;
    BOOST_REQUIRE(file.open());
    record_map table(file, 2u, 7u);
    BOOST_REQUIRE(table.create());

    const key_type key1{ { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef } };
    const key_type key2{ { 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b } };
    const key_type invalid{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } };

    const auto writer1 = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
    };

    const auto writer2 = [](byte_serializer& serial)
    {
        serial.write_byte(99);
        serial.write_byte(98);
        serial.write_byte(97);
        serial.write_byte(96);
        serial.write_byte(95);
        serial.write_byte(94);
        serial.write_byte(93);
    };

    // Allocate, create and store a new elements.
    auto element = table.allocator();
    /*const auto link1 =*/ element.create(key1, writer1);
    table.link(element);
    const auto link2 = element.create(key2, writer2);
    table.link(element);
    const auto link3 = element.create(key2, writer2);
    table.link(element);

    BOOST_REQUIRE(table.unlink(key1));
    BOOST_REQUIRE(table.unlink(key2));
    BOOST_REQUIRE(!table.unlink(invalid));

    const auto reader = [](byte_deserializer& deserial)
    {
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 99);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 98);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 97);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 96);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 95);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 94);
        BOOST_REQUIRE_EQUAL(deserial.read_byte(), 93);
    };

    // Key1 has been unlinked.
    BOOST_REQUIRE(!table.find(key1));

    // Second key2 has been unlinked, but is still present in the store.
    BOOST_REQUIRE(table.find(link3));

    // There is only one instance of key2 (next is not_found).
    auto const_element = table.find(key2);
    BOOST_REQUIRE(const_element);
    BOOST_REQUIRE_EQUAL(const_element.link(), link2);
    BOOST_REQUIRE_EQUAL(const_element.next(), record_map::not_found);
    BOOST_REQUIRE(const_element.match(key2));
    const_element.read(reader);
}

BOOST_AUTO_TEST_SUITE_END()
