/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include "../../test.hpp"
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(puts_tests)

using namespace system;
const table::puts::record expected0{};
const table::puts::record expected1
{
    {}, // schema::puts [all const static members]
    std_vector<uint64_t>
    {
        0x0000007856341211_u64
    },
    std_vector<uint64_t>
    {
    }
};
const table::puts::record expected2
{
    {}, // schema::puts [all const static members]
    std_vector<uint64_t>
    {
        0x0000007856341221_u64
    },
    std_vector<uint64_t>
    {
        0x0000007856341222_u64
    }
};
const table::puts::record expected3
{
    {}, // schema::puts [all const static members]
    std_vector<uint64_t>
    {
        0x0000007856341231_u64
    },
    std_vector<uint64_t>
    {
        0x0000007856341232_u64,
        0x0000007856341233_u64
    }
};
const data_chunk expected_file
{
    // record0 (empty set)

    // --------------------------------------------------------------------------------------------

    // record1
    0x11, 0x12, 0x34, 0x56, 0x78,

    // --------------------------------------------------------------------------------------------

    // record2
    0x21, 0x12, 0x34, 0x56, 0x78,
    0x22, 0x12, 0x34, 0x56, 0x78,

    // --------------------------------------------------------------------------------------------

    // record3
    0x31, 0x12, 0x34, 0x56, 0x78,
    0x32, 0x12, 0x34, 0x56, 0x78,
    0x33, 0x12, 0x34, 0x56, 0x78
};

BOOST_AUTO_TEST_CASE(puts__put__get__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::puts instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.put_link(expected0).is_terminal());
    BOOST_REQUIRE(!instance.put_link(expected1).is_terminal());
    BOOST_REQUIRE(!instance.put_link(expected2).is_terminal());
    BOOST_REQUIRE(!instance.put_link(expected3).is_terminal());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::puts::record record0{};
    BOOST_REQUIRE(instance.get(0, record0));
    BOOST_REQUIRE(record0 == expected0);

    table::puts::record record1{};
    record1.in_fks.resize(1);
    BOOST_REQUIRE(instance.get(0, record1));
    BOOST_REQUIRE(record1.in_fks == expected1.in_fks);
    BOOST_REQUIRE(record1.out_fks.empty());

    table::puts::record record2{};
    record2.in_fks.resize(1);
    record2.out_fks.resize(1);
    BOOST_REQUIRE(instance.get(1, record2));
    BOOST_REQUIRE(record2.in_fks == expected2.in_fks);
    BOOST_REQUIRE(record2.out_fks == expected2.out_fks);

    table::puts::record record3{};
    record3.in_fks.resize(1);
    record3.out_fks.resize(2);
    BOOST_REQUIRE(instance.get(3, record3));
    BOOST_REQUIRE(record3.in_fks == record3.in_fks);
    BOOST_REQUIRE(record3.out_fks == record3.out_fks);
}

BOOST_AUTO_TEST_SUITE_END()
