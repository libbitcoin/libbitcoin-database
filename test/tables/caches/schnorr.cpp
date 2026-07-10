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
////using namespace test;
////
////// Shared test vectors.
////constexpr auto header_fk = 0x00abcdef_u32;
////constexpr ec_xonly xonly_a = base16_array
////(
////    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////);
////constexpr ec_xonly xonly_b = base16_array
////(
////    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////);
////constexpr hash_digest digest_a = base16_hash
////(
////    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////);
////constexpr hash_digest digest_b = base16_hash
////(
////    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////);
////constexpr ec_signature sig_a = base16_array
////(
////    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////);
////constexpr ec_signature sig_b = base16_array
////(
////    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////);
////
////// schnorr (aggregate)
////// ----------------------------------------------------------------------------
////
////using schnorr_table = table::schnorr<chunk_storages>;
////using schnorr_storage = default_storage<table::schnorr_storage<chunk_storages>>;
////
////BOOST_AUTO_TEST_CASE(schnorr__create_verify_close__aggregate__expected)
////{
////    schnorr_storage head{ "head" };
////    schnorr_storage body{ "body" };
////    schnorr_table instance{ head, body };
////
////    BOOST_REQUIRE(instance.create());
////    BOOST_REQUIRE(instance.verify());
////    BOOST_REQUIRE(instance.close());
////}
////
////// Round-trip a single signature through the aggregate, mirroring the
////// set_signature write sequence: allocate one terminal correlate row, expand
////// subordinates, then commit real values. Correlate is header_fk only.
////BOOST_AUTO_TEST_CASE(schnorr__set_signature__single__expected)
////{
////    using correlate = table::schnorr_correlate::put_ref;
////    using digest_t = table::schnorr_digest::put_ref;
////    using xonly_t = table::schnorr_xonly::put_ref;
////    using signature_t = table::schnorr_signature::put_ref;
////
////    schnorr_storage head{ "head" };
////    schnorr_storage body{ "body" };
////    schnorr_table instance{ head, body };
////    BOOST_REQUIRE(instance.create());
////
////    const auto fk = instance.allocate(one);
////    BOOST_REQUIRE_EQUAL(fk, 0u);
////
////    BOOST_REQUIRE(instance.digest.put(fk, digest_t{ {}, digest_a }));
////    BOOST_REQUIRE(instance.xonly.put(fk, xonly_t{ {}, xonly_a }));
////    BOOST_REQUIRE(instance.signature.put(fk, signature_t{ {}, sig_a }));
////    BOOST_REQUIRE(instance.correlate.put(fk, correlate{ {}, header_fk }));
////
////    // Correlate: header_fk(3).
////    const auto expected_correlate = base16_chunk("efcdab");
////    const auto expected_digest = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////    const auto expected_xonly = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////    const auto expected_signature = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////
////    BOOST_REQUIRE_EQUAL(body.buffers_.at(0), expected_correlate);
////    BOOST_REQUIRE_EQUAL(body.buffers_.at(1), expected_digest);
////    BOOST_REQUIRE_EQUAL(body.buffers_.at(2), expected_xonly);
////    BOOST_REQUIRE_EQUAL(body.buffers_.at(3), expected_signature);
////    BOOST_REQUIRE(instance.close());
////}
////
////// schnorr_correlate (single writer + reader)
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_correlate__put_ref__single__expected)
////{
////    using putter = table::schnorr_correlate::put_ref;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_correlate instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk("efcdab");
////    BOOST_REQUIRE(instance.put(putter{ {}, header_fk }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// schnorr_correlate (multiple writer: threshold) + reader
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_correlate__put_refs__between__expected)
////{
////    using putter = table::schnorr_correlate::put_refs;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_correlate instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "efcdab"
////        "efcdab"
////        "efcdab"
////    );
////
////    // put_refs{ {}, size, header_fk }.
////    BOOST_REQUIRE(instance.put(putter{ {}, 3, header_fk }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr_correlate__get__using_record__expected)
////{
////    auto head = base16_chunk("02000000");
////    auto body = base16_chunk
////    (
////        "efcdab"
////        "785634"
////    );
////
////    chunk_storage head_store{ head };
////    chunk_storage body_store{ body };
////    table::schnorr_correlate instance{ head_store, body_store };
////
////    table::schnorr_correlate::record out{};
////    BOOST_REQUIRE(instance.get(0u, out));
////    BOOST_REQUIRE(out.header_fk == header_fk);
////
////    BOOST_REQUIRE(instance.get(1u, out));
////    BOOST_REQUIRE(out.header_fk == 0x00345678_u32);
////}
////
////// schnorr_digest (single + multiple writers)
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_digest__put_ref__single__expected)
////{
////    using putter = table::schnorr_digest::put_ref;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_digest instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////
////    BOOST_REQUIRE(instance.put(putter{ {}, digest_a }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr_digest__put_refs__tuples__expected)
////{
////    using putter = table::schnorr_digest::put_refs;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_digest instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const ec_xonly point{};
////    const ec_signature sig{};
////    const chain::threshold::tuples_t tuples
////    {
////        { digest_a, point, sig },
////        { digest_b, point, sig }
////    };
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    BOOST_REQUIRE(instance.put(putter{ {}, tuples }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// schnorr_xonly (single + multiple writers)
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_xonly__put_ref__single__expected)
////{
////    using putter = table::schnorr_xonly::put_ref;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_xonly instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////
////    BOOST_REQUIRE(instance.put(putter{ {}, xonly_a }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr_xonly__put_refs__tuples__expected)
////{
////    using putter = table::schnorr_xonly::put_refs;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_xonly instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const ec_signature sig{};
////    const chain::threshold::tuples_t tuples
////    {
////        { null_hash, xonly_a, sig },
////        { null_hash, xonly_b, sig }
////    };
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    BOOST_REQUIRE(instance.put(putter{ {}, tuples }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////// schnorr_signature (single + multiple writers)
////// ----------------------------------------------------------------------------
////
////BOOST_AUTO_TEST_CASE(schnorr_signature__put_ref__single__expected)
////{
////    using putter = table::schnorr_signature::put_ref;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_signature instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////    );
////
////    BOOST_REQUIRE(instance.put(putter{ {}, sig_a }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_CASE(schnorr_signature__put_refs__tuples__expected)
////{
////    using putter = table::schnorr_signature::put_refs;
////
////    chunk_storage head_store{};
////    chunk_storage body_store{};
////    table::schnorr_signature instance{ head_store, body_store };
////    BOOST_REQUIRE(instance.create());
////
////    const ec_xonly point{};
////    const chain::threshold::tuples_t tuples
////    {
////        { null_hash, point, sig_a },
////        { null_hash, point, sig_b }
////    };
////
////    const auto expected = base16_chunk
////    (
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
////    );
////
////    BOOST_REQUIRE(instance.put(putter{ {}, tuples }));
////    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
////}
////
////BOOST_AUTO_TEST_SUITE_END()
