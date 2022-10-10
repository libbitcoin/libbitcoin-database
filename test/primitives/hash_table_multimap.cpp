/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include "../utility/storage.hpp"
#include "../utility/utility.hpp"

BOOST_AUTO_TEST_SUITE(hash_table_multimap_tests)

BOOST_AUTO_TEST_CASE(hash_table_multimap__find__not_existing__not_found)
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

    test::storage hash_table_file;
    BOOST_REQUIRE(hash_table_file.map());
    record_map table(hash_table_file, 100u, sizeof(link_type));
    BOOST_REQUIRE(table.create());

    // Create the file and initialize index.
    test::storage index_file;
    BOOST_REQUIRE(index_file.map());
    record_manager index(index_file, 0, record_multimap::size(value_size));

    // Create the multimap.
    record_multimap multimap(table, index);

    // Test finding missing element.
    BOOST_REQUIRE(!multimap.find(key));
}

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
    BOOST_REQUIRE(hash_table_file.map());
    record_map table(hash_table_file, 100u, sizeof(link_type));
    BOOST_REQUIRE(table.create());

    // Create the file and initialize index.
    test::storage index_file;
    BOOST_REQUIRE(index_file.map());
    record_manager index(index_file, 0, record_multimap::size(value_size));

    // Create the multimap.
    record_multimap multimap(table, index);

    // Test find/remove of missing element.
    BOOST_REQUIRE(!multimap.find(key));
    BOOST_REQUIRE(!multimap.unlink(key));

    const auto writer1 = [](system::writer& sink) NOEXCEPT
    {
        sink.write_byte(110);
        sink.write_byte(4);
        sink.write_byte(99);
    };

    const auto writer2 = [](system::writer& sink) NOEXCEPT
    {
        sink.write_byte(100);
        sink.write_byte(40);
        sink.write_byte(9);
    };

    auto element = multimap.allocator();
    const auto link = element.create(writer1);
    multimap.link(key, element);

    const auto found = multimap.find(key);
    BOOST_REQUIRE(found);
    const auto found_from_link = multimap.get(link);
    BOOST_REQUIRE(found_from_link);
    BOOST_REQUIRE_EQUAL(found_from_link.link(), link);


    // Second element on the same key.
    auto element2 = multimap.allocator();
    const auto link2 = element2.create(writer2);
    multimap.link(key, element2);

    const auto found2 = multimap.find(key);
    BOOST_REQUIRE(found2);

    // Elements in index are correctly linked.
    BOOST_REQUIRE_EQUAL(found2.next(), found.link());

    // Read the two elements.
    const auto reader1 = [](system::reader& source) NOEXCEPT
    {
        BOOST_REQUIRE_EQUAL(source.read_byte(), 110u);
        BOOST_REQUIRE_EQUAL(source.read_byte(), 4u);
        BOOST_REQUIRE_EQUAL(source.read_byte(), 99u);
    };

    const auto reader2 = [](system::reader& source) NOEXCEPT
    {
        BOOST_REQUIRE_EQUAL(source.read_byte(), 100u);
        BOOST_REQUIRE_EQUAL(source.read_byte(), 40u);
        BOOST_REQUIRE_EQUAL(source.read_byte(), 9u);
    };

    found.read(reader1, 3);
    found2.read(reader2, 3);

    // Stored elements from index.
    BOOST_REQUIRE(index.get(link));
    BOOST_REQUIRE(index.get(link2));

    // Unlink once.
    BOOST_REQUIRE(multimap.unlink(key));

    BOOST_REQUIRE(multimap.find(key));
    BOOST_REQUIRE(multimap.get(link));

    // Unlink the second time.
    BOOST_REQUIRE(multimap.unlink(key));

    BOOST_REQUIRE(!multimap.find(key));
    BOOST_REQUIRE(!multimap.get(link));
}

BOOST_AUTO_TEST_SUITE_END()
