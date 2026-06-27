/////**
//// * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
////BOOST_AUTO_TEST_SUITE(ecdsa_tests)
////
////using namespace system;
////
////const table::ecdsa::record record1
////{
////    {},
////    base16_hash("1111111111111111111111111111111111111111111111111111111111111111"),
////    base16_array("222222222222222222222222222222222222222222222222222222222222222222"),
////    base16_array("33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"),
////    0x78,           // pair
////    0x5678_u16,     // group (16-bit)
////    0x00345678_u32  // header_fk (3 bytes used)
////};
////
////const table::ecdsa::record record2
////{
////    {},
////    base16_hash("4444444444444444444444444444444444444444444444444444444444444444"),
////    base16_array("555555555555555555555555555555555555555555555555555555555555555555"),
////    base16_array("66666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666"),
////    0x12,           // pair
////    0xcdef_u16,     // group
////    0x00cdef12_u32  // header_fk
////};
////
////const auto expected_head = base16_chunk("00000000");
////const auto closed_head   = base16_chunk("02000000");
////
////const auto expected_body = base16_chunk
////(
////    // record 1
////    "1111111111111111111111111111111111111111111111111111111111111111"
////    "222222222222222222222222222222222222222222222222222222222222222222"
////    "33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"
////    "78"            // pair
////    "7856"          // group (little-endian, 2 bytes)
////    "785634"        // header_fk (3 bytes)
////
////    // record 2
////    "4444444444444444444444444444444444444444444444444444444444444444"
////    "555555555555555555555555555555555555555555555555555555555555555555"
////    "66666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666"
////    "12"            // pair
////    "efcd"          // group
////    "12efcd"        // header_fk
////);
////
////BOOST_AUTO_TEST_CASE(ecdsa__put__two__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::ecdsa instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    table::ecdsa::link link1{};
////    BOOST_REQUIRE(instance.put_link(link1, record1));
////    BOOST_REQUIRE_EQUAL(link1, 0u);
////
////    table::ecdsa::link link2{};
////    BOOST_REQUIRE(instance.put_link(link2, record2));
////    BOOST_REQUIRE_EQUAL(link2, 1u);
////
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
////    BOOST_REQUIRE(instance.close());
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
////}
////
////BOOST_AUTO_TEST_CASE(ecdsa__get__two__expected)
////{
////    auto head = expected_head;
////    auto body = expected_body;
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::ecdsa instance{ head_store, body_store };
////
////    table::ecdsa::record out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(out == record1);
////    BOOST_REQUIRE(instance.get(1u, out));
////    BOOST_REQUIRE(out == record2);
////}
////
////BOOST_AUTO_TEST_CASE(ecdsa__truncate__from_two__expected)
////{
////    auto head = expected_head;
////    auto body = expected_body;
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::ecdsa instance{ head_store, body_store };
////
////    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
////    BOOST_REQUIRE(instance.truncate(1));
////    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
////
////    table::ecdsa::record out{};
////    BOOST_REQUIRE(!instance.get(1u, out));
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(out == record1);
////
////    BOOST_REQUIRE(instance.truncate(0));
////    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
////    BOOST_REQUIRE(!instance.get(0u, out));
////}
////
////// ecdsa_digest
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(ecdsa_digest__put_ref__repeated_digest__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::ecdsa_digest instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////
////    constexpr auto digest = base16_hash("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
////
////    using putter = table::ecdsa_digest::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, 3_size, 1_size, digest }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// ecdsa_compressed
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(ecdsa_compressed__put_ref__selected_keys__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::ecdsa_compressed instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    const system::ec_compresseds keys
////    {
////        base16_array("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
////        base16_array("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
////    };
////
////    using putter = table::ecdsa_compressed::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, keys, 1_size }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// ecdsa_signature
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(ecdsa_signature__put_ref__selected_sigs__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::ecdsa_signature instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    const system::ec_signatures signatures
////    {
////        base16_array
////        (
////            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        ),
////        base16_array
////        (
////            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////        )
////    };
////
////    using putter = table::ecdsa_signature::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, 2_size, signatures }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// ecdsa_correlate (writer + reader)
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(ecdsa_correlate__put_ref__packed_pairs__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::ecdsa_correlate instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "00" "7856" "785634"   // pair 0|0
////        "01" "7856" "785634"   // pair 0|1
////        "02" "7856" "785634"   // pair 0|2
////    );
////
////    using putter = table::ecdsa_correlate::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, 3_size, 1_size, 0x5678_u16, 0x00345678_u32 }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_CASE(ecdsa_correlate__get__using_record__expected)
////{
////    auto head = base16_chunk("03000000");
////    auto body = base16_chunk
////    (
////        "00" "7856" "785634"   // pair 0|0
////        "01" "7856" "785634"   // pair 0|1
////        "10" "7856" "785634"   // pair 1|1
////    );
////
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::ecdsa_correlate instance{ head_store, body_store };
////
////    table::ecdsa_correlate::record out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE_EQUAL(out.pair, 0x00);
////    BOOST_REQUIRE_EQUAL(out.group, 0x5678_u16);
////    BOOST_REQUIRE_EQUAL(out.header_fk, 0x00345678_u32);
////
////    BOOST_REQUIRE(instance.get(1u, out));
////    BOOST_REQUIRE_EQUAL(out.pair, 0x01);
////
////    BOOST_REQUIRE(instance.get(2u, out));
////    BOOST_REQUIRE_EQUAL(out.pair, 0x10);
////}
////
////BOOST_AUTO_TEST_SUITE_END()
