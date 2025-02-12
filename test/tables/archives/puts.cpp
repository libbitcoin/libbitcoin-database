/////**
//// * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#include "../../test.hpp"
////#include "../../mocks/chunk_storage.hpp"
////
////BOOST_AUTO_TEST_SUITE(puts_tests)
////
////using namespace system;
////const table::puts::slab expected0{};
////const table::puts::slab expected1
////{
////    {}, // schema::puts [all const static members]
////    std::vector<uint32_t>
////    {
////        0x56341211_u32
////    },
////    std::vector<uint64_t>
////    {
////    }
////};
////const table::puts::slab expected2
////{
////    {}, // schema::puts [all const static members]
////    std::vector<uint32_t>
////    {
////        0x56341221_u32
////    },
////    std::vector<uint64_t>
////    {
////        0x0000007856341222_u64
////    }
////};
////const table::puts::slab expected3
////{
////    {}, // schema::puts [all const static members]
////    std::vector<uint32_t>
////    {
////        0x56341231_u32
////    },
////    std::vector<uint64_t>
////    {
////        0x0000007856341232_u64,
////        0x0000007856341233_u64
////    }
////};
////const data_chunk expected_file
////{
////    // expected1 (0)
////    0x11, 0x12, 0x34, 0x56,
////
////    // expected2 (4)
////    0x21, 0x12, 0x34, 0x56,
////    0x22, 0x12, 0x34, 0x56, 0x78,
////
////    // expected3 (13)
////    0x31, 0x12, 0x34, 0x56,
////    0x32, 0x12, 0x34, 0x56, 0x78,
////    0x33, 0x12, 0x34, 0x56, 0x78
////};
////
////BOOST_AUTO_TEST_CASE(puts__put__get__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::puts instance{ head_store, body_store };
////    BOOST_REQUIRE(!instance.put_link(expected0).is_terminal());
////    BOOST_REQUIRE(!instance.put_link(expected1).is_terminal());
////    BOOST_REQUIRE(!instance.put_link(expected2).is_terminal());
////    BOOST_REQUIRE(!instance.put_link(expected3).is_terminal());
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);
////
////    table::puts::slab slab0{};
////    BOOST_REQUIRE(instance.get(0, slab0));
////    BOOST_REQUIRE(slab0 == expected0);
////
////    table::puts::slab slab1{};
////    slab1.point_fks.resize(1);
////    BOOST_REQUIRE(instance.get(0, slab1));
////    BOOST_REQUIRE(slab1.point_fks == expected1.point_fks);
////    BOOST_REQUIRE(slab1.out_fks.empty());
////
////    table::puts::slab slab2{};
////    slab2.point_fks.resize(1);
////    slab2.out_fks.resize(1);
////    BOOST_REQUIRE(instance.get(4, slab2));
////    BOOST_REQUIRE(slab2.point_fks == expected2.point_fks);
////    BOOST_REQUIRE(slab2.out_fks == expected2.out_fks);
////
////    table::puts::slab slab3{};
////    slab3.point_fks.resize(1);
////    slab3.out_fks.resize(2);
////    BOOST_REQUIRE(instance.get(13, slab3));
////    BOOST_REQUIRE(slab3.point_fks == expected3.point_fks);
////    BOOST_REQUIRE(slab3.out_fks == expected3.out_fks);
////}
////
////BOOST_AUTO_TEST_SUITE_END()
