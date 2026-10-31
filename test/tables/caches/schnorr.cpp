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

BC_AUTO_TEST_SUITE(schnorr_tests)

using namespace system;
using namespace test;

// Shared test vectors.
constexpr auto header_fk = 0x00abcdef_u32;
constexpr auto header_fk2 = 0x00345678_u32;
constexpr ec_xonly xonly_a = base16_array
(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
);
constexpr ec_xonly xonly_b = base16_array
(
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
);
constexpr hash_digest digest_a = base16_hash
(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
);
constexpr hash_digest digest_b = base16_hash
(
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
);
constexpr ec_signature sig_a = base16_array
(
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
);
constexpr ec_signature sig_b = base16_array
(
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
);

// schnorr (aggregate)
// ----------------------------------------------------------------------------

using schnorr_table = table::schnorr<chunk_storages>;
using schnorr_storage = default_storage<table::schnorr_storage<chunk_storages>>;

BC_AUTO_TEST_CASE(schnorr__create_verify_close__aggregate__expected)
{
    schnorr_storage head{ "head" };
    schnorr_storage body{ "body" };
    schnorr_table instance{ head, body };

    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE(instance.close());
}

// Round-trip signatures through the aggregate, mirroring the keyed
// set_signature sequence: allocate rows, then commit values per row.
// Correlate is header_fk only. Two rows pin column alignment (position i
// in every column belongs to row i), on which batch evaluation depends.
BC_AUTO_TEST_CASE(schnorr__set_signature__allocated_rows__expected)
{
    using correlate = table::schnorr_correlate::put_ref;
    using digest_t = table::schnorr_digest::put_ref;
    using xonly_t = table::schnorr_xonly::put_ref;
    using signature_t = table::schnorr_signature::put_ref;

    schnorr_storage head{ "head" };
    schnorr_storage body{ "body" };
    schnorr_table instance{ head, body };
    BC_REQUIRE(instance.create());

    // Allocate two rows (cursor model), write each independently.
    auto fk = instance.allocate(two);
    BC_REQUIRE_EQUAL(fk, 0u);

    BC_REQUIRE(instance.digest.put(fk, digest_t{ {}, digest_a }));
    BC_REQUIRE(instance.xonly.put(fk, xonly_t{ {}, xonly_a }));
    BC_REQUIRE(instance.signature.put(fk, signature_t{ {}, sig_a }));
    BC_REQUIRE(instance.correlate.put(fk, correlate{ {}, header_fk }));

    ++fk;
    BC_REQUIRE(instance.digest.put(fk, digest_t{ {}, digest_b }));
    BC_REQUIRE(instance.xonly.put(fk, xonly_t{ {}, xonly_b }));
    BC_REQUIRE(instance.signature.put(fk, signature_t{ {}, sig_b }));
    BC_REQUIRE(instance.correlate.put(fk, correlate{ {}, header_fk2 }));

    // Correlate: header_fk(3) per row.
    const auto expected_correlate = base16_chunk
    (
        "efcdab"
        "785634"
    );
    const auto expected_digest = base16_chunk
    (
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    );
    const auto expected_xonly = base16_chunk
    (
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    );
    const auto expected_signature = base16_chunk
    (
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    );

    BC_REQUIRE_EQUAL(body.buffers_.at(0), expected_correlate);
    BC_REQUIRE_EQUAL(body.buffers_.at(1), expected_digest);
    BC_REQUIRE_EQUAL(body.buffers_.at(2), expected_xonly);
    BC_REQUIRE_EQUAL(body.buffers_.at(3), expected_signature);
    BC_REQUIRE(instance.close());
}

BC_AUTO_TEST_SUITE_END()
