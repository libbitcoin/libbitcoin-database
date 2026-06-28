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
////BOOST_AUTO_TEST_SUITE(schnorr_tests)
////
////using namespace system;
////
////const table::schnorr::record record1
////{
////    {},
////    base16_hash("1111111111111111111111111111111111111111111111111111111111111111"),
////    base16_array("2222222222222222222222222222222222222222222222222222222222222222"),
////    base16_array("33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"),
////    chain::threshold::category_t::single,
////    0x5678_u16,     // pair
////    0x1234_u16,     // group
////    0x00cdef12_u32  // header_fk
////};
////
////const table::schnorr::record record2
////{
////    {},
////    base16_hash("4444444444444444444444444444444444444444444444444444444444444444"),
////    base16_array("5555555555555555555555555555555555555555555555555555555555555555"),
////    base16_array("66666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666"),
////    chain::threshold::category_t::between,
////    0xcdef_u16,     // pair
////    0x5678_u16,     // group
////    0x00345678_u32  // header_fk
////};
////
////const auto expected_head = base16_chunk("00000000");
////const auto closed_head   = base16_chunk("02000000");
////const auto expected_body = base16_chunk
////(
////    // record 1
////    "1111111111111111111111111111111111111111111111111111111111111111"
////    "2222222222222222222222222222222222222222222222222222222222222222"
////    "33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333"
////    "01"          // category (checksig/single)
////    "7856"        // pair
////    "3412"        // group
////    "12efcd"      // header_fk (3 bytes)
////
////    // record 2
////    "4444444444444444444444444444444444444444444444444444444444444444"
////    "5555555555555555555555555555555555555555555555555555555555555555"
////    "66666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666"
////    "08"          // category (within/between)
////    "efcd"        // pair
////    "7856"        // group
////    "785634"      // header_fk
////);
////
////BOOST_AUTO_TEST_CASE(schnorr__put__two__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::schnorr instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    table::schnorr::link link1{};
////    BOOST_REQUIRE(instance.put_link(link1, record1));
////    BOOST_REQUIRE_EQUAL(link1, 0u);
////
////    table::schnorr::link link2{};
////    BOOST_REQUIRE(instance.put_link(link2, record2));
////    BOOST_REQUIRE_EQUAL(link2, 1u);
////
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
////    BOOST_REQUIRE(instance.close());
////    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr__get__two__expected)
////{
////    auto head = expected_head;
////    auto body = expected_body;
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::schnorr instance{ head_store, body_store };
////
////    table::schnorr::record out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(out == record1);
////    BOOST_REQUIRE(instance.get(1u, out));
////    BOOST_REQUIRE(out == record2);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr__truncate__from_two__expected)
////{
////    auto head = expected_head;
////    auto body = expected_body;
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::schnorr instance{ head_store, body_store };
////
////    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
////    BOOST_REQUIRE(instance.truncate(1));
////    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
////
////    table::schnorr::record out{};
////    BOOST_REQUIRE(!instance.get(1u, out));
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(out == record1);
////
////    BOOST_REQUIRE(instance.truncate(0));
////    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
////    BOOST_REQUIRE(!instance.get(0u, out));
////}
////
////// schnorr_digest
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_digest__put_ref__multiple_digests__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::schnorr_digest instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    const ec_xonly dummy_point{};
////    const ec_signature dummy_sig{};
////    constexpr auto hash1 = base16_hash("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
////    constexpr auto hash2 = base16_hash("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
////
////    const chain::threshold::tuples_t tuples
////    {
////        { hash1, dummy_point, dummy_sig },
////        { hash2, dummy_point, dummy_sig }
////    };
////
////    using putter = table::schnorr_digest::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, tuples }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// schnorr_xonly
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_xonly__put_ref__multiple_points__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::schnorr_xonly instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    const ec_signature dummy_sig{};
////    constexpr auto xonly1 = base16_array("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
////    constexpr auto xonly2 = base16_array("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
////
////    const chain::threshold::tuples_t tuples
////    {
////        { null_hash, xonly1, dummy_sig },
////        { null_hash, xonly2, dummy_sig }
////    };
////
////    using putter = table::schnorr_xonly::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, tuples }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// schnorr_signature
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_signature__put_ref__multiple_signatures__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::schnorr_signature instance{ head_store, body_store };
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
////    const ec_xonly dummy_point{};
////    constexpr auto signature1 = base16_array
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////    constexpr auto signature2 = base16_array
////    (
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    const chain::threshold::tuples_t tuples
////    {
////        { null_hash, dummy_point, signature1 },
////        { null_hash, dummy_point, signature2 }
////    };
////
////    using putter = table::schnorr_signature::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, tuples }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// schnorr_correlate (writer + reader)
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_correlate__put_ref__single_category__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::schnorr_correlate instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "01" "0000" "3412" "efcdab"
////        "00" "0000" "3412" "efcdab"
////        "00" "0000" "3412" "efcdab"
////    );
////
////    const ec_xonly dummy_point{};
////    const ec_signature dummy_sig{};
////    const chain::threshold batch
////    {
////        .tuples =
////        {
////            { null_hash, dummy_point, dummy_sig },
////            { null_hash, dummy_point, dummy_sig },
////            { null_hash, dummy_point, dummy_sig }
////        },
////        .category = chain::threshold::category_t::single
////    };
////
////    using putter = table::schnorr_correlate::put_ref;
////    BOOST_REQUIRE(instance.put(putter{ {}, batch, 0x1234_u16, 0x00abcdef_u32 }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr_correlate__get__using_record__expected)
////{
////    auto head = base16_chunk("02000000");
////    auto body = base16_chunk
////    (
////        "01" "0000" "3412" "efcdab"
////        "02" "0000" "3412" "efcdab"
////    );
////
////    test::chunk_storage head_store{ head };
////    test::chunk_storage body_store{ body };
////    table::schnorr_correlate instance{ head_store, body_store };
////
////    table::schnorr_correlate::record out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE_EQUAL(out.category, chain::threshold::category_t::single);
////    BOOST_REQUIRE_EQUAL(out.pair, 0_u16);
////    BOOST_REQUIRE_EQUAL(out.group, 0x1234_u16);
////    BOOST_REQUIRE_EQUAL(out.header_fk, 0x00abcdef_u32);
////
////    BOOST_REQUIRE(instance.get(1u, out));
////    BOOST_REQUIRE_EQUAL(out.category, chain::threshold::category_t::equal);
////    BOOST_REQUIRE_EQUAL(out.pair, 0_u16);
////}
////
////BOOST_AUTO_TEST_SUITE_END()
