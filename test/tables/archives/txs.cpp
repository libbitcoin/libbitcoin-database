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
#include "../../mocks/dfile.hpp"

BOOST_AUTO_TEST_SUITE(txs_tests)

using namespace system;
constexpr search<schema::txs::sk> key = base16_array("112233");
const table::txs::slab expected0{};
const table::txs::slab expected1
{
    {}, // schema::txs [all const static members]
    std_vector<uint32_t>
    {
        0x56341211_u32
    }
};
const table::txs::slab expected2
{
    {}, // schema::txs [all const static members]
    std_vector<uint32_t>
    {
        0x56341221_u32,
        0x56341222_u32
    }
};
const table::txs::slab expected3
{
    {}, // schema::txs [all const static members]
    std_vector<uint32_t>
    {
        0x56341231_u32,
        0x56341232_u32,
        0x56341233_u32
    }
};
const data_chunk expected_file
{
    // 00->terminal
    0xff, 0xff, 0xff, 0xff,

    // key
    0x11, 0x22, 0x33,

    // slab0 (count) [0]
    0x00, 0x00, 0x00, 0x00,

    // slab0 (txs)

    // --------------------------------------------------------------------------------------------

    // 11->00
    0x00, 0x00, 0x00, 0x00,

    // key
    0x11, 0x22, 0x33,

    // slab1 (count) [1]
    0x01, 0x00, 0x00, 0x00,

    // slab1 (txs)
    0x11, 0x12, 0x34, 0x56,

    // --------------------------------------------------------------------------------------------

    // 26->11
    0x0b, 0x00, 0x00, 0x00,

    // key
    0x11, 0x22, 0x33,

    // slab2 (count) [2]
    0x02, 0x00, 0x00, 0x00,

    // slab2
    0x21, 0x12, 0x34, 0x56,
    0x22, 0x12, 0x34, 0x56,

    // --------------------------------------------------------------------------------------------

    // 45->26
    0x1a, 0x00, 0x00, 0x00,

    // key
    0x11, 0x22, 0x33,

    // slab3 (count) [3]
    0x03, 0x00, 0x00, 0x00,

    // slab3
    0x31, 0x12, 0x34, 0x56,
    0x32, 0x12, 0x34, 0x56,
    0x33, 0x12, 0x34, 0x56
};

BOOST_AUTO_TEST_CASE(txs__put__get__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::txs instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.put_link(key, expected0).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, expected1).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, expected2).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, expected3).is_terminal());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::txs::slab slab{};
    BOOST_REQUIRE(instance.get(0, slab));
    BOOST_REQUIRE(slab == expected0);

    BOOST_REQUIRE(instance.get(11, slab));
    BOOST_REQUIRE(slab == expected1);

    BOOST_REQUIRE(instance.get(26, slab));
    BOOST_REQUIRE(slab == expected2);

    BOOST_REQUIRE(instance.get(45, slab));
    BOOST_REQUIRE(slab == expected3);
}

BOOST_AUTO_TEST_SUITE_END()
