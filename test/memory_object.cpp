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
#include <boost/test/unit_test.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;

BOOST_AUTO_TEST_SUITE(memory_object_tests)

BOOST_AUTO_TEST_CASE(slab__test)
{
    data_base::touch_file("slabs");
    memory_map file("slabs");
    BITCOIN_ASSERT(file.access()->buffer() != nullptr);
    file.allocate(200);
    slab_manager data(file, 0);
    data.create();

    data.start();

    file_offset position = data.new_slab(100);
    BOOST_REQUIRE(position == 8);
    //slab_byte_pointer slab = data.get(position);

    file_offset position2 = data.new_slab(100);
    BOOST_REQUIRE(position2 == 108);
    //slab = data.get(position2);

    BOOST_REQUIRE(file.size() >= 208);
}

BOOST_AUTO_TEST_CASE(array__test)
{
    data_base::touch_file("array");
    memory_map file("array");
    BITCOIN_ASSERT(file.access()->buffer() != nullptr);
    file.allocate(4 + 4 * 10);

    hash_table<uint32_t, uint32_t> array(file, 0);
    array.create(10);
    array.start();

    array.write(9, 110);
    BOOST_REQUIRE(array.read(9) == 110);
}

BOOST_AUTO_TEST_CASE(record__test)
{
    data_base::touch_file("records");
    memory_map file("records");
    BITCOIN_ASSERT(file.access()->buffer() != nullptr);
    file.allocate(4);
    record_manager recs(file, 0, 10);
    recs.create();

    recs.start();

    array_index idx = recs.new_records(1);
    BOOST_REQUIRE(idx == 0);
    idx = recs.new_records(1);
    BOOST_REQUIRE(idx == 1);
    BOOST_REQUIRE(file.size() >= 2 * 10 + 4);
    recs.sync();
}

BOOST_AUTO_TEST_CASE(record_list__test)
{
    data_base::touch_file("record_list");
    memory_map file("record_list");
    BITCOIN_ASSERT(file.access()->buffer() != nullptr);
    file.allocate(4);
    BC_CONSTEXPR size_t record_size = record_list_offset + 6;
    record_manager recs(file, 0, record_size);
    recs.create();

    recs.start();
    record_list lrs(recs);

    array_index idx = lrs.create();
    BOOST_REQUIRE(idx == 0);
    array_index idx1 = lrs.create();
    BOOST_REQUIRE(idx1 == 1);
    idx = lrs.create();
    BOOST_REQUIRE(idx == 2);

    idx = lrs.insert(idx1);
    BOOST_REQUIRE(idx == 3);
    idx = lrs.insert(idx);
    BOOST_REQUIRE(idx == 4);

    size_t count = 0;
    array_index valid = idx;
    while (idx != record_list::empty)
    {
        valid = idx;
        idx = lrs.next(idx);
        ++count;
    }
    BOOST_REQUIRE(count == 3);
    BOOST_REQUIRE(valid == idx1);
    recs.sync();
}

BOOST_AUTO_TEST_CASE(slab_hash_table__test)
{
    data_base::touch_file("slab_hash_table");
    memory_map file("slab_hash_table");
    BITCOIN_ASSERT(file.access()->buffer() != nullptr);
    file.allocate(4 + 8 * 100 + 8);

    slab_hash_table_header header(file, 0);
    header.create(100);
    header.start();

    slab_manager alloc(file, 4 + 8 * 100);
    alloc.create();
    alloc.start();

    typedef byte_array<4> tiny_hash;
    slab_hash_table<tiny_hash> ht(header, alloc);

    auto write = [](uint8_t* data)
    {
        data[0] = 110;
        data[1] = 110;
        data[2] = 4;
        data[3] = 99;
    };
    ht.store(tiny_hash{ { 0xde, 0xad, 0xbe, 0xef } }, write, 8);
    const auto memory1 = ht.find(tiny_hash{ { 0xde, 0xad, 0xbe, 0xef } });
    const auto slab1 = memory1->buffer();
    BOOST_REQUIRE(slab1);
    BOOST_REQUIRE(slab1[0] == 110);
    BOOST_REQUIRE(slab1[1] == 110);
    BOOST_REQUIRE(slab1[2] == 4);
    BOOST_REQUIRE(slab1[3] == 99);

    const auto memory2 = ht.find(tiny_hash{ { 0xde, 0xad, 0xbe, 0xee } });
    const auto slab2 = memory1->buffer();
    BOOST_REQUIRE(slab2);
}

BOOST_AUTO_TEST_SUITE_END()

