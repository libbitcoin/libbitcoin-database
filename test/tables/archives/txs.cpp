/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(txs_tests)

using namespace system;
const table::txs::slab slab0
{
    {}, // schema::txs [all const static members]
    0x000000,
    std::vector<uint32_t>
    {
    }
};
const table::txs::slab slab1
{
    {}, // schema::txs [all const static members]
    0x0000ab,
    std::vector<uint32_t>
    {
        0x56341211_u32
    }
};
const table::txs::slab slab2
{
    {}, // schema::txs [all const static members]
    0x00a00b,
    std::vector<uint32_t>
    {
        0x56341221_u32,
        0x56341222_u32
    }
};
const table::txs::slab slab3
{
    {}, // schema::txs [all const static members]
    0xa0000b,
    std::vector<uint32_t>
    {
        0x56341231_u32,
        0x56341232_u32,
        0x56341233_u32
    }
};
const data_chunk expected0
{
    // slab0 (count) [0]
    0x00, 0x00, 0x00,

    // slab0 (wire) [0x00]
    0x00, 0x00, 0x00,

    // slab0 (txs)
};
const data_chunk expected1
{
    // slab1 (count) [1]
    0x01, 0x00, 0x00,

    // slab1 (wire) [0x0000ab]
    0xab, 0x00, 0x00,

    // slab1 (txs)
    0x11, 0x12, 0x34, 0x56,
};
const data_chunk expected2
{
    // slab2 (count) [2]
    0x02, 0x00, 0x00,

    // slab2 (wire) [0x00a00b]
    0x0b, 0xa0, 0x00,

    // slab2
    0x21, 0x12, 0x34, 0x56,
    0x22, 0x12, 0x34, 0x56,
};
const data_chunk expected3
{
    // slab3 (count) [3]
    0x03, 0x00, 0x00,

    // slab3 (wire) [0xa0000b]
    0x0b, 0x00, 0xa0,

    // slab3
    0x31, 0x12, 0x34, 0x56,
    0x32, 0x12, 0x34, 0x56,
    0x33, 0x12, 0x34, 0x56
};

BOOST_AUTO_TEST_CASE(txs__put__get__expected)
{
    constexpr size_t key = 3;
    table::txs::slab slab{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::txs instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.exists(key));

    BOOST_REQUIRE(instance.put(key, slab0));
    BOOST_REQUIRE(instance.exists(key));
    BOOST_REQUIRE(instance.at(key, slab));
    BOOST_REQUIRE(slab == slab0);
    BOOST_REQUIRE(instance.get(instance.at(key), slab));
    BOOST_REQUIRE(slab == slab0);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), build_chunk({ expected0 }));

    BOOST_REQUIRE(instance.put(key, slab1));
    BOOST_REQUIRE(instance.exists(key));
    BOOST_REQUIRE(instance.at(key, slab));
    BOOST_REQUIRE(slab == slab1);
    BOOST_REQUIRE(instance.get(instance.at(key), slab));
    BOOST_REQUIRE(slab == slab1);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), build_chunk({ expected0, expected1 }));

    BOOST_REQUIRE(instance.put(key, slab2));
    BOOST_REQUIRE(instance.exists(key));
    BOOST_REQUIRE(instance.at(key, slab));
    BOOST_REQUIRE(slab == slab2);
    BOOST_REQUIRE(instance.get(instance.at(key), slab));
    BOOST_REQUIRE(slab == slab2);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), build_chunk({ expected0, expected1, expected2 }));

    BOOST_REQUIRE(instance.put(key, slab3));
    BOOST_REQUIRE(instance.exists(key));
    BOOST_REQUIRE(instance.at(key, slab));
    BOOST_REQUIRE(slab == slab3);
    BOOST_REQUIRE(instance.get(instance.at(key), slab));
    BOOST_REQUIRE(slab == slab3);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), build_chunk({ expected0, expected1, expected2, expected3 }));
}

BOOST_AUTO_TEST_SUITE_END()
