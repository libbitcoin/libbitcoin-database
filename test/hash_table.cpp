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
#include <random>
#include <boost/functional/hash_fwd.hpp>
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

typedef byte_array<4> tiny_hash;
typedef byte_array<8> little_hash;

// Extend std namespace with tiny_hash wrapper.
namespace std
{

template <>
struct hash<tiny_hash>
{
    size_t operator()(const tiny_hash& value) const
    {
        return boost::hash_range(value.begin(), value.end());
    }
};

template <>
struct hash<little_hash>
{
    size_t operator()(const little_hash& value) const
    {
        return boost::hash_range(value.begin(), value.end());
    }
};

} // namspace std

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

    store::create(DIRECTORY "/slab_hash_table__write_read");
    memory_map file(DIRECTORY "/slab_hash_table__write_read");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(header_size + minimum_slabs_size);

    slab_hash_table_header header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    const file_offset slab_start = header_size;

    slab_manager alloc(file, slab_start);
    BOOST_REQUIRE(alloc.create());
    BOOST_REQUIRE(alloc.start());

    slab_hash_table<hash_digest> ht(header, alloc);

    std::default_random_engine engine;
    for (size_t i = 0; i < total_txs; ++i)
    {
        data_chunk value = generate_random_bytes(engine, tx_size);
        hash_digest key = bitcoin_hash(value);
        auto write = [&value](serializer<uint8_t*>& serial)
        {
            serial.write_forward(value);
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
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);

    slab_hash_table_header header(file, buckets);
    BOOST_REQUIRE(header.start());
    BOOST_REQUIRE(header.size() == buckets);

    const auto slab_start = slab_hash_table_header_size(buckets);

    slab_manager alloc(file, slab_start);
    BOOST_REQUIRE(alloc.start());

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
    store::create(DIRECTORY "/slab_hash_table");
    memory_map file(DIRECTORY "/slab_hash_table");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(4 + 8 * 100 + 8);

    slab_hash_table_header header(file, 100);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    slab_manager alloc(file, 4 + 8 * 100);
    BOOST_REQUIRE(alloc.create());
    BOOST_REQUIRE(alloc.start());

    slab_hash_table<tiny_hash> ht(header, alloc);
    const auto write = [](serializer<uint8_t*>& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(99);
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

    store::create(DIRECTORY "/record_hash_table__32bit");
    memory_map file(DIRECTORY "/record_hash_table__32bit");
    BOOST_REQUIRE(file.open());

    // Cannot hold an address reference because of following resize operation.
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(header_size + minimum_records_size);

    record_hash_table_header header(file, record_buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    typedef byte_array<4> tiny_hash;
    BC_CONSTEXPR size_t record_size = hash_table_record_size<tiny_hash>(4);
    const file_offset records_start = header_size;

    record_manager alloc(file, records_start, record_size);
    BOOST_REQUIRE(alloc.create());
    BOOST_REQUIRE(alloc.start());

    record_hash_table<tiny_hash> ht(header, alloc);
    tiny_hash key{ { 0xde, 0xad, 0xbe, 0xef } };
    tiny_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b } };

    const auto write = [](serializer<uint8_t*>& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
    };

    const auto write1 = [](serializer<uint8_t*>& serial)
    {
        serial.write_byte(99);
        serial.write_byte(98);
        serial.write_byte(97);
        serial.write_byte(96);
    };

    // [e][e]
    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
    BOOST_REQUIRE_EQUAL(header.read(1), header.empty);

    ht.store(key, write);
    alloc.sync();

    // [0][e]
    BOOST_REQUIRE_EQUAL(header.read(0), 0u);
    BOOST_REQUIRE_EQUAL(header.read(1), header.empty);

    ht.store(key, write);
    alloc.sync();

    // [1->0][e]
    BOOST_REQUIRE_EQUAL(header.read(0), 1u);

    ht.store(key1, write1);
    alloc.sync();

    // [1->0][2]
    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
    BOOST_REQUIRE_EQUAL(header.read(1), 2u);

    ht.store(key1, write);
    alloc.sync();

    // [1->0][3->2]
    BOOST_REQUIRE_EQUAL(header.read(0), 1u);
    BOOST_REQUIRE_EQUAL(header.read(1), 3u);

    // Verify 0->empty
    record_row<tiny_hash> item0(alloc, 0);
    BOOST_REQUIRE_EQUAL(item0.next_index(), header.empty);

    // Verify 1->0
    record_row<tiny_hash> item1(alloc, 1);
    BOOST_REQUIRE_EQUAL(item1.next_index(), 0u);

    // Verify 2->empty
    record_row<tiny_hash> item2(alloc, 2);
    BOOST_REQUIRE_EQUAL(item2.next_index(), header.empty);

    // Verify 3->2
    record_row<tiny_hash> item3(alloc, 3);
    BOOST_REQUIRE_EQUAL(item3.next_index(), 2u);

    // [X->0][3->2]
    BOOST_REQUIRE(ht.unlink(key));
    alloc.sync();

    BOOST_REQUIRE_EQUAL(header.read(0), 0);
    BOOST_REQUIRE_EQUAL(header.read(1), 3u);

    // Verify 0->empty
    record_row<tiny_hash> item0a(alloc, 0);
    BOOST_REQUIRE_EQUAL(item0a.next_index(), header.empty);

    // Verify 3->2
    record_row<tiny_hash> item3a(alloc, 3);
    BOOST_REQUIRE_EQUAL(item3a.next_index(), 2u);

    // Verify 2->empty
    record_row<tiny_hash> item2a(alloc, 2);
    BOOST_REQUIRE_EQUAL(item2a.next_index(), header.empty);

    // [0][X->2]
    BOOST_REQUIRE(ht.unlink(key1));
    alloc.sync();

    BOOST_REQUIRE_EQUAL(header.read(0), 0u);
    BOOST_REQUIRE_EQUAL(header.read(1), 2u);

    // Verify 0->empty
    record_row<tiny_hash> item0b(alloc, 0);
    BOOST_REQUIRE_EQUAL(item0b.next_index(), header.empty);

    // Verify 2->empty
    record_row<tiny_hash> item2b(alloc, 2);
    BOOST_REQUIRE_EQUAL(item2b.next_index(), header.empty);

    tiny_hash invalid{ { 0x00, 0x01, 0x02, 0x03 } };
    BOOST_REQUIRE(!ht.unlink(invalid));
}

BOOST_AUTO_TEST_CASE(record_hash_table__64bit__test)
{
    BC_CONSTEXPR size_t record_buckets = 2;
    BC_CONSTEXPR size_t header_size = record_hash_table_header_size(record_buckets);

    store::create(DIRECTORY "/record_hash_table_64bit");
    memory_map file(DIRECTORY "/record_hash_table_64bit");
    BOOST_REQUIRE(file.open());

    // Cannot hold an address reference because of following resize operation.
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(header_size + minimum_records_size);

    record_hash_table_header header(file, record_buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    typedef byte_array<8> little_hash;
    BC_CONSTEXPR size_t record_size = hash_table_record_size<little_hash>(8);
    const file_offset records_start = header_size;

    record_manager alloc(file, records_start, record_size);
    BOOST_REQUIRE(alloc.create());
    BOOST_REQUIRE(alloc.start());

    record_hash_table<little_hash> ht(header, alloc);

    little_hash key{ { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef } };
    little_hash key1{ { 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b } };

    const auto write = [](serializer<uint8_t*>& serial)
    {
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
        serial.write_byte(110);
        serial.write_byte(110);
        serial.write_byte(4);
        serial.write_byte(88);
    };

    const auto write1 = [](serializer<uint8_t*>& serial)
    {
        serial.write_byte(99);
        serial.write_byte(98);
        serial.write_byte(97);
        serial.write_byte(96);
        serial.write_byte(95);
        serial.write_byte(94);
        serial.write_byte(93);
        serial.write_byte(92);
    };

    ht.store(key, write);
    alloc.sync();

    // [e][0]
    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
    BOOST_REQUIRE_EQUAL(header.read(1), 0u);

    ht.store(key, write);
    alloc.sync();

    // [e][1->0]
    BOOST_REQUIRE_EQUAL(header.read(0), header.empty);
    BOOST_REQUIRE_EQUAL(header.read(1), 1u);

    ht.store(key1, write1);
    alloc.sync();

    // [2][1->0]
    BOOST_REQUIRE_EQUAL(header.read(0), 2u);
    BOOST_REQUIRE_EQUAL(header.read(1), 1u);

    ht.store(key1, write);
    alloc.sync();

    // [3->2][1->0]
    BOOST_REQUIRE_EQUAL(header.read(0), 3u);
    BOOST_REQUIRE_EQUAL(header.read(1), 1u);

    record_row<little_hash> item(alloc, 3);
    BOOST_REQUIRE_EQUAL(item.next_index(), 2u);

    record_row<little_hash> item1(alloc, 2);
    BOOST_REQUIRE_EQUAL(item1.next_index(), header.empty);

    // [3->2][X->0]
    BOOST_REQUIRE(ht.unlink(key));
    alloc.sync();

    BOOST_REQUIRE_EQUAL(header.read(1), 0u);

    // [X->2][X->0]
    BOOST_REQUIRE(ht.unlink(key1));
    alloc.sync();

    BOOST_REQUIRE_EQUAL(header.read(0), 2u);

    little_hash invalid{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } };
    BOOST_REQUIRE(!ht.unlink(invalid));
}

BOOST_AUTO_TEST_SUITE_END()

