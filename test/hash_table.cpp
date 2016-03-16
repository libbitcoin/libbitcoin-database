/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <random>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;

BC_CONSTEXPR size_t total_txs = 200;
BC_CONSTEXPR size_t tx_size = 200;
BC_CONSTEXPR size_t buckets = 100;
#define DIRECTORY "hash_table"

data_chunk generate_random_bytes(std::default_random_engine& engine,
    size_t size)
{
    data_chunk result(size);
    for (uint8_t& byte: result)
        byte = engine() % std::numeric_limits<uint8_t>::max();

    return result;
}

void create_database_file()
{
    BC_CONSTEXPR size_t header_size = slab_hash_table_header_size(buckets);

    data_base::touch_file(DIRECTORY "/slab_hash_table__write_read");
    memory_map file(DIRECTORY "/slab_hash_table__write_read");
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(header_size + minimum_slabs_size);

    slab_hash_table_header header(file, buckets);
    header.create();
    header.start();

    const file_offset slab_start = header_size;

    slab_manager alloc(file, slab_start);
    alloc.create();
    alloc.start();

    slab_hash_table<hash_digest> ht(header, alloc);

    std::default_random_engine engine;
    for (size_t i = 0; i < total_txs; ++i)
    {
        data_chunk value = generate_random_bytes(engine, tx_size);
        hash_digest key = bitcoin_hash(value);
        auto write = [&value](memory_ptr data)
        {
            std::copy(value.begin(), value.end(), REMAP_ADDRESS(data));
        };
        ht.store(key, write, value.size());
    }

    alloc.sync();
}

class hash_table_directory_setup_fixture
{
public:
    hash_table_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
    }

    ////~hash_table_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(hash_table_tests, hash_table_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(slab_hash_table__write_read__test)
{
    // Create the data file to be read below.
    create_database_file();

    memory_map file(DIRECTORY "/slab_hash_table__write_read");
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);

    slab_hash_table_header header(file, buckets);
    header.start();

    BOOST_REQUIRE(header.size() == buckets);

    const auto slab_start = slab_hash_table_header_size(buckets);

    slab_manager alloc(file, slab_start);
    alloc.start();

    slab_hash_table<hash_digest> ht(header, alloc);

    std::default_random_engine engine;
    for (size_t i = 0; i < total_txs; ++i)
    {
        const auto value = generate_random_bytes(engine, tx_size);
        const auto key = bitcoin_hash(value);
        const auto memory = ht.find(key);
        const auto slab = REMAP_ADDRESS(memory);

        BOOST_REQUIRE(slab);
        BOOST_REQUIRE(std::equal(value.begin(), value.end(), slab));
    }
}

BOOST_AUTO_TEST_CASE(slab_hash_table__test)
{
    data_base::touch_file(DIRECTORY "/slab_hash_table");
    memory_map file(DIRECTORY "/slab_hash_table");
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(4 + 8 * 100 + 8);

    slab_hash_table_header header(file, 100);
    header.create();
    header.start();

    slab_manager alloc(file, 4 + 8 * 100);
    alloc.create();
    alloc.start();

    typedef byte_array<4> tiny_hash;
    slab_hash_table<tiny_hash> ht(header, alloc);

    const auto write = [](memory_ptr data)
    {
        const auto address = REMAP_ADDRESS(data);
        address[0] = 110;
        address[1] = 110;
        address[2] = 4;
        address[3] = 99;
    };
    ht.store(tiny_hash{ { 0xde, 0xad, 0xbe, 0xef } }, write, 8);
    const auto memory1 = ht.find(tiny_hash{ { 0xde, 0xad, 0xbe, 0xef } });
    const auto slab1 = REMAP_ADDRESS(memory1);
    BOOST_REQUIRE(slab1);
    BOOST_REQUIRE(slab1[0] == 110);
    BOOST_REQUIRE(slab1[1] == 110);
    BOOST_REQUIRE(slab1[2] == 4);
    BOOST_REQUIRE(slab1[3] == 99);

    const auto memory2 = ht.find(tiny_hash{ { 0xde, 0xad, 0xbe, 0xee } });
    const auto slab2 = REMAP_ADDRESS(memory1);
    BOOST_REQUIRE(slab2);
}

BOOST_AUTO_TEST_CASE(record_hash_table__32bit__test)
{
    BC_CONSTEXPR size_t record_buckets = 2;
    BC_CONSTEXPR size_t header_size = record_hash_table_header_size(record_buckets);

    data_base::touch_file(DIRECTORY "/record_hash_table__32bit");
    memory_map file(DIRECTORY "/record_hash_table__32bit");

    // Cannot hold an address reference because of following resize operation.
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(header_size + minimum_records_size);

    record_hash_table_header header(file, record_buckets);
    header.create();
    header.start();

    typedef byte_array<4> tiny_hash;
    BC_CONSTEXPR size_t record_size = hash_table_record_size<tiny_hash>(4);
    const file_offset records_start = header_size;

    record_manager alloc(file, records_start, record_size);
    alloc.create();
    alloc.start();

    record_hash_table<tiny_hash> ht(header, alloc);

    tiny_hash key{ { 0xde, 0xad, 0xbe, 0xef } };
    const auto write = [](memory_ptr data)
    {
        const auto address = REMAP_ADDRESS(data);
        address[0] = 110;
        address[1] = 110;
        address[2] = 4;
        address[3] = 88;
    };
    ht.store(key, write);

    tiny_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b } };
    const auto write1 = [](memory_ptr data)
    {
        const auto address = REMAP_ADDRESS(data);
        address[0] = 99;
        address[1] = 98;
        address[2] = 97;
        address[3] = 96;
    };
    ht.store(key, write);
    ht.store(key1, write1);
    ht.store(key1, write);

    alloc.sync();

    BOOST_REQUIRE(header.read(0) == header.empty);
    BOOST_REQUIRE(header.read(1) == 3);

    record_row<tiny_hash> item(alloc, 3);
    BOOST_REQUIRE(item.next_index() == 2);
    record_row<tiny_hash> item1(alloc, 2);
    BOOST_REQUIRE(item1.next_index() == 1);

    // Should unlink record 1
    BOOST_REQUIRE(ht.unlink(key));

    BOOST_REQUIRE(header.read(1) == 3);
    record_row<tiny_hash> item2(alloc, 2);
    BOOST_REQUIRE(item2.next_index() == 0);

    // Should unlink record 3 from buckets
    BOOST_REQUIRE(ht.unlink(key1));

    BOOST_REQUIRE(header.read(1) == 2);

    tiny_hash invalid{ { 0x00, 0x01, 0x02, 0x03 } };
    BOOST_REQUIRE(!ht.unlink(invalid));
}

BOOST_AUTO_TEST_CASE(record_hash_table_header__64bit__test)
{
    BC_CONSTEXPR size_t record_buckets = 2;
    BC_CONSTEXPR size_t header_size = record_hash_table_header_size(record_buckets);

    data_base::touch_file(DIRECTORY "/record_hash_table_64bit");
    memory_map file(DIRECTORY "/record_hash_table_64bit");

    // Cannot hold an address reference because of following resize operation.
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(header_size + minimum_records_size);

    record_hash_table_header header(file, record_buckets);
    header.create();
    header.start();

    typedef byte_array<8> tiny_hash;
    BC_CONSTEXPR size_t record_size = hash_table_record_size<tiny_hash>(8);
    const file_offset records_start = header_size;

    record_manager alloc(file, records_start, record_size);
    alloc.create();
    alloc.start();

    record_hash_table<tiny_hash> ht(header, alloc);

    tiny_hash key{ { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef } };
    const auto write = [](memory_ptr data)
    {
        const auto address = REMAP_ADDRESS(data);
        address[0] = 110;
        address[1] = 110;
        address[2] = 4;
        address[3] = 88;
        address[4] = 110;
        address[5] = 110;
        address[6] = 4;
        address[7] = 88;
    };
    ht.store(key, write);

    tiny_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b } };
    const auto write1 = [](memory_ptr data)
    {
        const auto address = REMAP_ADDRESS(data);
        address[0] = 99;
        address[1] = 98;
        address[2] = 97;
        address[3] = 96;
        address[4] = 95;
        address[5] = 94;
        address[6] = 93;
        address[7] = 92;
    };
    ht.store(key, write);
    ht.store(key1, write1);
    ht.store(key1, write);

    alloc.sync();

    BOOST_REQUIRE(header.read(0) == header.empty);
    BOOST_REQUIRE(header.read(1) == 3);

    record_row<tiny_hash> item(alloc, 3);
    BOOST_REQUIRE(item.next_index() == 2);
    record_row<tiny_hash> item1(alloc, 2);
    BOOST_REQUIRE(item1.next_index() == 1);

    // Should unlink record 1
    BOOST_REQUIRE(ht.unlink(key));

    BOOST_REQUIRE(header.read(1) == 3);
    record_row<tiny_hash> item2(alloc, 2);
    BOOST_REQUIRE(item2.next_index() == 0);

    // Should unlink record 3 from buckets
    BOOST_REQUIRE(ht.unlink(key1));

    BOOST_REQUIRE(header.read(1) == 2);

    tiny_hash invalid{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } };
    BOOST_REQUIRE(!ht.unlink(invalid));
}

BOOST_AUTO_TEST_SUITE_END()

