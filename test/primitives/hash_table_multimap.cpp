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

BOOST_AUTO_TEST_SUITE(hash_table_multimap_tests)

BOOST_AUTO_TEST_CASE(hash_table_multimap__construct__always__expected)
{
    // Define multimap type.
    typedef test::tiny_hash key_type;
    typedef uint32_t index_type;
    typedef uint32_t link_type;
    typedef record_manager<link_type> record_manager;
    typedef hash_table<record_manager, index_type, link_type, key_type> record_map;
    typedef hash_table_multimap<index_type, link_type, key_type> record_multimap;

    const auto value_size = 3u;
    const key_type key{ { 0xde, 0xad, 0xbe, 0xef } };

    // Create the file and initialize hash table.
    test::storage hash_table_file;
    BOOST_REQUIRE(hash_table_file.open());
    record_map table(hash_table_file, 100u, sizeof(link_type));
    BOOST_REQUIRE(table.create());

    // Create the file and initialize index.
    test::storage index_file;
    BOOST_REQUIRE(index_file.open());
    record_manager index(index_file, 0, record_multimap::size(value_size));

    // Create the multimap.
    record_multimap multimap(table, index);

    // Test find/remove of missing element.
    BOOST_REQUIRE(!multimap.find(key));
    BOOST_REQUIRE(!multimap.unlink(key));

    const auto writer = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(99);
    };

    auto element = multimap.allocator();
    /*const auto link =*/ element.create(writer);
    multimap.link(key, element);

    BOOST_REQUIRE(multimap.find(key));
    BOOST_REQUIRE(multimap.unlink(key));
    BOOST_REQUIRE(!multimap.find(key));
}

BOOST_AUTO_TEST_SUITE_END()
