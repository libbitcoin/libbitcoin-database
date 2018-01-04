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

#include <utility>
#include <bitcoin/database.hpp>
#include "../utility/test_map.hpp"
#include "../utility/utility.hpp"

using namespace bc;
using namespace bc::database;

BOOST_AUTO_TEST_SUITE(slab_hash_table_tests)

BOOST_AUTO_TEST_CASE(slab_hash_table__store_find__three_bytes__success)
{
    const auto buckets = 100u;

    // Open/close storage and manage flush/remap locking externaly.
    test::test_map file;
    BOOST_REQUIRE(file.open());

    // Create and initialize header bucket count and empty buckets.
    slab_hash_table_header header(file, buckets);
    BOOST_REQUIRE(header.create());

    // Create the initial slab (size) space after the hash table header.
    slab_manager manager(file, slab_hash_table_header_size(buckets));
    BOOST_REQUIRE(manager.create());

    // Join header and slab manager into a hash table.
    slab_hash_table<test::tiny_hash> table(header, manager);

    const auto writer = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(99);
    };
    const test::tiny_hash key{ { 0xde, 0xad, 0xbe, 0xef } };
    table.store(key, writer, 3);
    const auto memory = table.find(key);
    const auto slab = memory->buffer();
    BOOST_REQUIRE(slab);
    BOOST_REQUIRE_EQUAL(slab[0], 110u);
    BOOST_REQUIRE_EQUAL(slab[1], 4u);
    BOOST_REQUIRE_EQUAL(slab[2], 99u);
}

BOOST_AUTO_TEST_CASE(slab_hash_table__store_find__random_bytes_multiple_times__test)
{
    const auto buckets = 100u;

    // Open/close storage and manage flush/remap locking externaly.
    test::test_map file;
    BOOST_REQUIRE(file.open());

    // Create and initialize header bucket count and empty buckets.
    slab_hash_table_header header(file, buckets);
    BOOST_REQUIRE(header.create());

    // Create the initial slab (size) space after the hash table header.
    slab_manager manager(file, slab_hash_table_header_size(buckets));
    BOOST_REQUIRE(manager.create());

    // Join header and slab manager into a hash table.
    slab_hash_table<hash_digest> table(header, manager);

    std::default_random_engine engine;
    for (size_t i = 0; i < 50; ++i)
    {
        const auto value = test::generate_random_bytes(engine, 200);
        const auto key = bitcoin_hash(value);
        const auto writer = [&](byte_serializer& serial)
        {
            serial.write_bytes(value);
        };
        table.store(key, writer, value.size());
        const auto memory = table.find(key);
        const auto slab = memory->buffer();
        BOOST_REQUIRE(slab);
        BOOST_REQUIRE(std::equal(value.begin(), value.end(), slab));
    }
}

BOOST_AUTO_TEST_SUITE_END()
