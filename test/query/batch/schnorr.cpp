/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_batch_ecdsa_tests, test::directory_setup_fixture)

using namespace system;

const hash_digest sighash_bad = base16_array
(
    "4242424242424242424242424242424242424242424242424242424242424242"
);
const hash_digest schnorr_sighash = base16_array
(
    "0000000000000000000000000000000000000000000000000000000000000000"
);
const ec_xonly schnorr_xonly = base16_array
(
    "f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9"
);
const ec_signature schnorr_signature = base16_array
(
    "e907831f80848d1069a5371b402410364bdf1c5f8307b0084c55f1ce2dca8215"
    "25f66a4a85ea8b71e482a74f382d2ce5ebeee8fdb2172f477df4900d310536c0"
);

// Commit one row as one block's accumulator.
static bool set_one(test::query_accessor& query, const hash_digest& digest,
    const ec_xonly& point, const ec_signature& sig,
    const header_link& link) NOEXCEPT
{
    chain::schnorr_signatures sigs{};
    sigs.append(digest, point, sig);
    return query.set_signatures(sigs, link);
}

BOOST_AUTO_TEST_CASE(query_batch_ecdsa__verify_schnorr_signatures__empty__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 0u);
    BOOST_REQUIRE(query.verify_schnorr_signatures({}, links));
    BOOST_REQUIRE(links.empty());
}

BOOST_AUTO_TEST_CASE(query_batch_ecdsa__verify_schnorr_signatures__one_valid__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 42));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 1u);
    BOOST_REQUIRE(query.verify_schnorr_signatures({}, links));
    BOOST_REQUIRE(links.empty());
}

BOOST_AUTO_TEST_CASE(query_batch_ecdsa__verify_schnorr_signatures__one_invalid__expected_link)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    constexpr auto expected = 42u;
    BOOST_REQUIRE(set_one(query, sighash_bad, schnorr_xonly, schnorr_signature, expected));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 1u);
    BOOST_REQUIRE(query.verify_schnorr_signatures({}, links));
    BOOST_REQUIRE_EQUAL(links.size(), 1u);
    BOOST_REQUIRE_EQUAL(links.front(), expected);
}

BOOST_AUTO_TEST_CASE(query_batch_ecdsa__verify_schnorr_signatures__various__expected_links)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    constexpr auto expected1 = 42u;
    constexpr auto expected2 = 24u;

    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 1));
    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 2));
    BOOST_REQUIRE(set_one(query, sighash_bad, schnorr_xonly, schnorr_signature, expected1));
    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 3));
    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 4));
    BOOST_REQUIRE(set_one(query, sighash_bad, schnorr_xonly, schnorr_signature, expected2));
    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 5));
    BOOST_REQUIRE(set_one(query, schnorr_sighash, schnorr_xonly, schnorr_signature, 6));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 8u);
    BOOST_REQUIRE(query.verify_schnorr_signatures({}, links));
    BOOST_REQUIRE_EQUAL(links.size(), 2u);

    const auto back = links.back();
    const auto front = links.front();
    BOOST_REQUIRE((front == expected1 && back == expected2) || (front == expected2 && back == expected1));
}

BOOST_AUTO_TEST_SUITE_END()
