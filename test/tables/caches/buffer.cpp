/////**
//// * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
////BOOST_AUTO_TEST_SUITE(buffer_tests)
////
////using namespace system;
////const chain::transaction empty{};
////const auto genesis = system::settings{ system::chain::selection::mainnet }.genesis_block;
////const auto& genesis_tx = *genesis.transactions_ptr()->front();
////const table::buffer::key key1{ 0x01, 0x02, 0x03, 0x04 };
////const table::buffer::key key2{ 0xa1, 0xa2, 0xa3, 0xa4 };
////const table::buffer::slab slab1{ {}, empty };
////const table::buffer::slab slab2{ {}, genesis_tx };
////const data_chunk expected_head = base16_chunk
////(
////    "0000000000"
////    "1300000000"
////    "ffffffffff"
////    "ffffffffff"
////    "ffffffffff"
////    "ffffffffff"
////);
////const data_chunk closed_head = base16_chunk
////(
////    "e800000000"
////    "1300000000"
////    "ffffffffff"
////    "ffffffffff"
////    "ffffffffff"
////    "ffffffffff"
////);
////const data_chunk expected_body = base16_chunk
////(
////    "ffffffffff"            // next->end
////    "01020304"              // key1
////    "00000000000000000000"  // tx1 (empty)
////
////    "0000000000"            // next->
////    "a1a2a3a4"              // key2
////    "0100000001000000000000000000000000000000000000000"
////    "0000000000000000000000000ffffffff4d04ffff001d0104"
////    "455468652054696d65732030332f4a616e2f3230303920436"
////    "8616e63656c6c6f72206f6e206272696e6b206f6620736563"
////    "6f6e64206261696c6f757420666f722062616e6b73fffffff"
////    "f0100f2052a01000000434104678afdb0fe5548271967f1a6"
////    "7130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4"
////    "cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6b"
////    "f11d5fac00000000"      // tx2 (genesis[0])
////);
////
////BOOST_AUTO_TEST_CASE(buffer__put__two__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::buffer instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE(instance.create());
////
////    table::buffer::link link1{};
////    BOOST_REQUIRE(instance.put_link(link1, key1, slab1));
////    BOOST_REQUIRE_EQUAL(link1, 0u);
////
////    table::buffer::link link2{};
////    BOOST_REQUIRE(instance.put_link(link2, key2, slab2));
////    BOOST_REQUIRE_EQUAL(link2, 0x13);
////
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
////    BOOST_REQUIRE(instance.close());
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
////}
////
////BOOST_AUTO_TEST_CASE(buffer__get__two__expected)
////{
////    auto head = expected_head;
////    auto body = expected_body;
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::buffer instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
////
////    table::buffer::slab out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(out == slab1);
////    BOOST_REQUIRE(instance.get(0x13, out));
////    BOOST_REQUIRE(out == slab2);
////}
////
////BOOST_AUTO_TEST_CASE(buffer__put__get__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::buffer instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE(instance.create());
////    BOOST_REQUIRE(!instance.put_link(key1, table::buffer::slab_ptr
////    {
////        {},
////        to_shared(chain::transaction{})
////    }).is_terminal());
////    BOOST_REQUIRE(!instance.put_link(key2, table::buffer::put_ref
////    {
////        {},
////        slab2.tx
////    }).is_terminal());
////
////    table::buffer::slab_ptr out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(*out.tx == slab1.tx);
////    BOOST_REQUIRE(instance.get(0x13, out));
////    BOOST_REQUIRE(*out.tx == slab2.tx);
////}
////
////BOOST_AUTO_TEST_SUITE_END()
