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

BOOST_AUTO_TEST_CASE(record_hash_table__32bit__test)
{
    typedef test::tiny_hash key_type;
    typedef record_hash_table<key_type, uint32_t, uint32_t> record_map;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto buckets = 2u;
    const auto value_size = 4u;
    record_map table(file, buckets, value_size);

    const key_type key{ { 0xde, 0xad, 0xbe, 0xef } };
    const key_type key1{ { 0xb0, 0x0b, 0xb0, 0x0b } };
    const key_type invalid{ { 0x00, 0x01, 0x02, 0x03 } };

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

    table.store(key, write);
    table.store(key, write);
    table.store(key1, write1);
    table.store(key1, write);
    BOOST_REQUIRE(table.unlink(key));
    BOOST_REQUIRE(table.unlink(key1));
    BOOST_REQUIRE(!table.unlink(invalid));
}

BOOST_AUTO_TEST_CASE(record_hash_table__64bit__test)
{
    typedef test::little_hash key_type;
    typedef record_hash_table<key_type, uint32_t, uint32_t> record_map;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto buckets = 2u;
    const auto value_size = 7u;
    record_map table(file, buckets, value_size);

    const key_type key{ { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef } };
    const key_type key1{ { 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b } };
    const key_type invalid{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } };

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
    table.store(key, write);
    table.store(key1, write1);
    table.store(key1, write);
    BOOST_REQUIRE(table.unlink(key));
    BOOST_REQUIRE(table.unlink(key1));

    BOOST_REQUIRE(!table.unlink(invalid));
}

BOOST_AUTO_TEST_SUITE_END()
