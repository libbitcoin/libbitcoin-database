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
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;

#define DIRECTORY "structure"

class structure_directory_setup_fixture
{
public:
    structure_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
    }

    ////~structure_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(structure_tests, structure_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(hash_table_header__test)
{
    store::create(DIRECTORY "/hash_table_header");
    memory_map file(DIRECTORY "/hash_table_header");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(4 + 4 * 10);

    hash_table_header<uint32_t, uint32_t> header(file, 10);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    header.write(9, 110);
    BOOST_REQUIRE(header.read(9) == 110);
}

BOOST_AUTO_TEST_CASE(slab_manager__test)
{
    store::create(DIRECTORY "/slab_manager");
    memory_map file(DIRECTORY "/slab_manager");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(200);

    slab_manager data(file, 0);
    BOOST_REQUIRE(data.create());
    BOOST_REQUIRE(data.start());

    file_offset position = data.new_slab(100);
    BOOST_REQUIRE(position == 8);
    //slab_byte_pointer slab = data.get(position);

    file_offset position2 = data.new_slab(100);
    BOOST_REQUIRE(position2 == 108);
    //slab = data.get(position2);

    BOOST_REQUIRE(file.size() >= 208);
}

BOOST_AUTO_TEST_CASE(record_manager__test)
{
    store::create(DIRECTORY "/record_manager");
    memory_map file(DIRECTORY "/record_manager");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);
    file.resize(4);

    record_manager recs(file, 0, 10);
    BOOST_REQUIRE(recs.create());
    BOOST_REQUIRE(recs.start());

    array_index idx = recs.new_records(1);
    BOOST_REQUIRE(idx == 0);
    idx = recs.new_records(1);
    BOOST_REQUIRE(idx == 1);
    BOOST_REQUIRE(file.size() >= 2 * 10 + 4);
    recs.sync();
}

BOOST_AUTO_TEST_CASE(record_list__test)
{
    store::create(DIRECTORY "/record_list");
    memory_map file(DIRECTORY "/record_list");
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(REMAP_ADDRESS(file.access()) != nullptr);

    file.resize(4);
    const size_t record_size = record_list_offset + 6;
    record_manager recs(file, 0, record_size);
    BOOST_REQUIRE(recs.create());
    BOOST_REQUIRE(recs.start());

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

BOOST_AUTO_TEST_SUITE_END()

