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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_signatures_tests, test::directory_setup_fixture)

using namespace system;

const hash_digest sighash_bad = base16_array(
    "4242424242424242424242424242424242424242424242424242424242424242");

// ecdsa

const hash_digest ecdsa_sighash = base16_array(
    "504d68beac187dd0b259ddd6ed6d5d6348150b9b23ee6dfdb43e87f74dd3c547");
const ec_compressed ecdsa_compressed = base16_array(
    "039cfcfe4a5d0efad27382e5d2b478eb398a8b691a66e01c878b600b5042b33166");
const ec_signature ecdsa_signature = base16_array(
    "b434c7c720d63d71e1136d740df7ff636770ce59cb0389ae8cd24c0d4441f143"
    "01f4e1dbc36f32b4683faeecc8e4b2c6810da69e98fd783f1aad105636c3da08");

BOOST_AUTO_TEST_CASE(query__verify_ecdsa_signatures__empty__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.ecdsa_records(), 0u);
    BOOST_REQUIRE(query.verify_ecdsa_signatures(links));
    BOOST_REQUIRE(links.empty());
}

BOOST_AUTO_TEST_CASE(query__verify_ecdsa_signatures__one_valid__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 42));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.ecdsa_records(), 1u);
    BOOST_REQUIRE(query.verify_ecdsa_signatures(links));
    BOOST_REQUIRE(links.empty());
}

BOOST_AUTO_TEST_CASE(query__verify_ecdsa_signatures__one_invalid__expected_link)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    constexpr auto expected = 42u;
    BOOST_REQUIRE(query.set_signature(sighash_bad, ecdsa_compressed, ecdsa_signature, expected));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.ecdsa_records(), 1u);
    BOOST_REQUIRE(query.verify_ecdsa_signatures(links));
    BOOST_REQUIRE_EQUAL(links.size(), 1u);
    BOOST_REQUIRE_EQUAL(links.front(), expected);
}

BOOST_AUTO_TEST_CASE(query__verify_ecdsa_signatures__various__expected_links)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    constexpr auto expected1 = 42u;
    constexpr auto expected2 = 24u;
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 1));
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 2));
    BOOST_REQUIRE(query.set_signature(sighash_bad,   ecdsa_compressed, ecdsa_signature, expected1));
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 3));
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 4));
    BOOST_REQUIRE(query.set_signature(sighash_bad,   ecdsa_compressed, ecdsa_signature, expected2));
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 5));
    BOOST_REQUIRE(query.set_signature(ecdsa_sighash, ecdsa_compressed, ecdsa_signature, 6));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.ecdsa_records(), 8u);
    BOOST_REQUIRE(query.verify_ecdsa_signatures(links));
    BOOST_REQUIRE_EQUAL(links.size(), 2u);

    // Order is not guaranteed.
    const auto back = links.back();
    const auto front = links.front();
    BOOST_REQUIRE((front == expected1 && back == expected2) || (front == expected2 && back == expected1));
}

// schnorr

// schnorr (valid BIP-340 test vector #0)
const hash_digest schnorr_sighash = base16_array(
    "0000000000000000000000000000000000000000000000000000000000000000");
const ec_xonly schnorr_xonly = base16_array(
    "f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9");
const ec_signature schnorr_signature = base16_array(
    "e907831f80848d1069a5371b402410364bdf1c5f8307b0084c55f1ce2dca8215"
    "25f66a4a85ea8b71e482a74f382d2ce5ebeee8fdb2172f477df4900d310536c0");

BOOST_AUTO_TEST_CASE(query__verify_schnorr_signatures__empty__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 0u);
    BOOST_REQUIRE(query.verify_schnorr_signatures(links));
    BOOST_REQUIRE(links.empty());
}

BOOST_AUTO_TEST_CASE(query__verify_schnorr_signatures__one_valid__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 42));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 1u);
    BOOST_REQUIRE(query.verify_schnorr_signatures(links));
    BOOST_REQUIRE(links.empty());
}

BOOST_AUTO_TEST_CASE(query__verify_schnorr_signatures__one_invalid__expected_link)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    constexpr auto expected = 42u;
    BOOST_REQUIRE(query.set_signature(sighash_bad, schnorr_xonly, schnorr_signature, expected));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 1u);
    BOOST_REQUIRE(query.verify_schnorr_signatures(links));
    BOOST_REQUIRE_EQUAL(links.size(), 1u);
    BOOST_REQUIRE_EQUAL(links.front(), expected);
}

BOOST_AUTO_TEST_CASE(query__verify_schnorr_signatures__various__expected_links)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    constexpr auto expected1 = 42u;
    constexpr auto expected2 = 24u;
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 1));
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 2));
    BOOST_REQUIRE(query.set_signature(sighash_bad,     schnorr_xonly, schnorr_signature, expected1));
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 3));
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 4));
    BOOST_REQUIRE(query.set_signature(sighash_bad,     schnorr_xonly, schnorr_signature, expected2));
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 5));
    BOOST_REQUIRE(query.set_signature(schnorr_sighash, schnorr_xonly, schnorr_signature, 6));

    header_links links{};
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), 8u);
    BOOST_REQUIRE(query.verify_schnorr_signatures(links));
    BOOST_REQUIRE_EQUAL(links.size(), 2u);

    // Order is not guaranteed.
    const auto back = links.back();
    const auto front = links.front();
    BOOST_REQUIRE((front == expected1 && back == expected2) || (front == expected2 && back == expected1));
}

BOOST_AUTO_TEST_SUITE_END()
