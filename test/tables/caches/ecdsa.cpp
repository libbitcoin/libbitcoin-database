/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

BOOST_AUTO_TEST_SUITE(ecdsa_tests)

using namespace system;
using namespace test;

// Shared test vectors.
constexpr auto group = 0x1234_u16;
constexpr auto header_fk = 0x00abcdef_u32;
constexpr hash_digest digest_a = base16_hash
(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
);
constexpr ec_compressed point_a = base16_array
(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
);
constexpr ec_compressed point_b = base16_array
(
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
);
constexpr ec_compressed point_c = base16_array
(
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
);
constexpr ec_signature sig_a = base16_array
(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
);

// ecdsa (aggregate)
// ----------------------------------------------------------------------------

using ecdsa_table = table::ecdsa<chunk_storages>;
using ecdsa_storage = default_storage<table::ecdsa_storage<chunk_storages>>;

BOOST_AUTO_TEST_CASE(ecdsa__create_verify_close__aggregate__expected)
{
    ecdsa_storage head{ "head" };
    ecdsa_storage body{ "body" };
    ecdsa_table instance{ head, body };

    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.close());
}

// Round-trip a single (1-of-1) signature through the aggregate, mirroring the
// set_signature write sequence: allocate one terminal correlate row, expand
// subordinates, then commit real values. Correlate is header_fk-first:
// header_fk(3) | pair(1) | group(2). Single encodes pair=0 (0|0 -> 1-of-1).
BOOST_AUTO_TEST_CASE(ecdsa__set_signature__single__expected)
{
    using correlate = table::ecdsa_correlate::put_ref;
    using row_t = table::ecdsa_row::put_ref;

    ecdsa_storage head{ "head" };
    ecdsa_storage body{ "body" };
    ecdsa_table instance{ head, body };
    BOOST_REQUIRE(instance.create());

    const auto fk = instance.allocate(one);
    BOOST_REQUIRE_EQUAL(fk, 0u);

    BOOST_REQUIRE(instance.row.put(fk, row_t{ {}, digest_a, point_a, sig_a }));
    BOOST_REQUIRE(instance.correlate.put(fk, correlate{ {}, header_fk, group }));

    // Correlate: header_fk(3) | pair=0 | group.
    const auto expected_correlate = base16_chunk("efcdab" "00" "3412");
    const auto expected_row = base16_chunk
    (
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    );

    BOOST_REQUIRE_EQUAL(body.buffers_.at(0), expected_correlate);
    BOOST_REQUIRE_EQUAL(body.buffers_.at(1), expected_row);
    BOOST_REQUIRE(instance.close());
}

// ecdsa_correlate (single writer + reader)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(ecdsa_correlate__put_ref__single__expected)
{
    using putter = table::ecdsa_correlate::put_ref;

    chunk_storage head_store{};
    chunk_storage body_store{};
    table::ecdsa_correlate instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    // Single writes header_fk | pair=0 | group.
    const auto expected = base16_chunk("efcdab" "00" "3412");

    BOOST_REQUIRE(instance.put(putter{ {}, header_fk, group }));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
}

// ecdsa_correlate (m-of-n writer: packed pairs) + reader
// ----------------------------------------------------------------------------

// 1-of-3: m=1 sig, n=3 keys -> ecdsa_count(1,3)=1*(3-1+1)=3 rows.
// Pairs pack_word(sig,key) for sig in [0,m), key in [sig, gap+sig]:
// (0,0),(0,1),(0,2) -> 0x00,0x01,0x02. Same group/header_fk per row.
BOOST_AUTO_TEST_CASE(ecdsa_correlate__put_refs__one_of_three__expected)
{
    using putter = table::ecdsa_correlate::put_refs;

    chunk_storage head_store{};
    chunk_storage body_store{};
    table::ecdsa_correlate instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    const auto expected = base16_chunk
    (
        "efcdab" "00" "3412"   // header_fk, pair 0|0, group
        "efcdab" "01" "3412"   // header_fk, pair 0|1, group
        "efcdab" "02" "3412"   // header_fk, pair 0|2, group
    );

    // put_refs{ {}, header_fk, keys, sigs, group }.
    BOOST_REQUIRE(instance.put(putter{ {}, 3_size, header_fk, 3_size, 1_size, group }));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
}

BOOST_AUTO_TEST_CASE(ecdsa_correlate__get__using_record__expected)
{
    auto head = base16_chunk("03000000");
    auto body = base16_chunk
    (
        "efcdab" "00" "3412"   // header_fk, pair 0|0, group
        "efcdab" "01" "3412"   // header_fk, pair 0|1, group
        "efcdab" "10" "3412"   // header_fk, pair 1|1, group
    );

    chunk_storage head_store{ head };
    chunk_storage body_store{ body };
    table::ecdsa_correlate instance{ head_store, body_store };

    table::ecdsa_correlate::record out{};
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out.header_fk == header_fk);
    BOOST_REQUIRE_EQUAL(out.pair, 0x00_u8);
    BOOST_REQUIRE_EQUAL(out.group, group);

    BOOST_REQUIRE(instance.get(1u, out));
    BOOST_REQUIRE_EQUAL(out.pair, 0x01_u8);

    BOOST_REQUIRE(instance.get(2u, out));
    BOOST_REQUIRE_EQUAL(out.pair, 0x10_u8);
}

// ecdsa_row (single + m-of-n writers)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(ecdsa_row__put_ref__single__expected)
{
    using putter = table::ecdsa_row::put_ref;

    chunk_storage head_store{};
    chunk_storage body_store{};
    table::ecdsa_row instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    const auto expected = base16_chunk
    (
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    );

    BOOST_REQUIRE(instance.put(putter{ {}, digest_a, point_a, sig_a }));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
}

// m-of-n walks the key matrix: for sig in [0,m), key in [sig, gap+sig], each
// row repeating the common digest and the sig across its key span.
// 1-of-3: gap=2, sig=0, keys 0,1,2 -> three rows of sig 0 with each key.
BOOST_AUTO_TEST_CASE(ecdsa_row__put_refs__matrix__expected)
{
    using putter = table::ecdsa_row::put_refs;

    chunk_storage head_store{};
    chunk_storage body_store{};
    table::ecdsa_row instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    const ec_compresseds keys{ point_a, point_b, point_c };
    const ec_signatures signatures{ sig_a };

    const auto expected = base16_chunk
    (
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    );

    BOOST_REQUIRE(instance.put(putter{ {}, 3_size, digest_a, keys, signatures }));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
