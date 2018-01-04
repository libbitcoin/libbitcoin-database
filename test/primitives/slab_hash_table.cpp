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
#include "../utility/utility.hpp"

using namespace bc;
using namespace bc::database;

#define DIRECTORY "slab_hash_table"
BC_CONSTEXPR size_t buckets = 100;
BC_CONSTEXPR size_t tx_size = 200;
BC_CONSTEXPR size_t total_txs = 200;

struct slab_hash_table_directory_setup_fixture
{
    slab_hash_table_directory_setup_fixture()
    {
        BOOST_REQUIRE(test::clear_path(DIRECTORY));
    }
};

BOOST_FIXTURE_TEST_SUITE(slab_hash_table_tests, slab_hash_table_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(slab_hash_table__write_read__test)
{
    // Create the data file to be read below.
    const auto path = DIRECTORY "/slab_hash_table__write_read";
    test::create_database_file(path, buckets, total_txs, tx_size);

    file_map file(path);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.access()->buffer() != nullptr);

    slab_hash_table_header header(file, buckets);
    BOOST_REQUIRE(header.start());
    BOOST_REQUIRE(header.size() == buckets);

    const auto slab_start = slab_hash_table_header_size(buckets);

    slab_manager maanager(file, slab_start);
    BOOST_REQUIRE(maanager.start());

    slab_hash_table<hash_digest> table(header, maanager);

    std::default_random_engine engine;
    for (size_t i = 0; i < total_txs; ++i)
    {
        const auto value = test::generate_random_bytes(engine, tx_size);
        const auto key = bitcoin_hash(value);
        const auto memory = table.find(key);
        const auto slab = memory->buffer();

        BOOST_REQUIRE(slab);
        BOOST_REQUIRE(std::equal(value.begin(), value.end(), slab));
    }
}

BOOST_AUTO_TEST_CASE(slab_hash_table__test)
{
    const auto path = DIRECTORY "/slab_hash_table";
    test::create(path);
    file_map file(path);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.access()->buffer() != nullptr);
    file.resize(4 + 8 * 100 + 8);

    slab_hash_table_header header(file, 100);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    slab_manager manager(file, 4 + 8 * 100);
    BOOST_REQUIRE(manager.create());
    BOOST_REQUIRE(manager.start());

    slab_hash_table<test::tiny_hash> table(header, manager);
    const auto write = [](byte_serializer& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(99);
    };
    table.store(test::tiny_hash{ { 0xde, 0xad, 0xbe, 0xef } }, write, 8);
    const auto memory1 = table.find(test::tiny_hash{ { 0xde, 0xad, 0xbe, 0xef } });
    const auto slab1 = memory1->buffer();
    BOOST_REQUIRE(slab1);
    BOOST_REQUIRE(slab1[0] == 110);
    BOOST_REQUIRE(slab1[1] == 110);
    BOOST_REQUIRE(slab1[2] == 4);
    BOOST_REQUIRE(slab1[3] == 99);

    const auto memory2 = table.find(test::tiny_hash{ { 0xde, 0xad, 0xbe, 0xee } });
    const auto slab2 = memory1->buffer();
    BOOST_REQUIRE(slab2);
}

BOOST_AUTO_TEST_SUITE_END()
