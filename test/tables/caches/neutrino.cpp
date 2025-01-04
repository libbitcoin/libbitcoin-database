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

BOOST_AUTO_TEST_SUITE(neutrino_tests)

using namespace system;
const table::neutrino::key key1{ 0x01, 0x02, 0x03 };
const table::neutrino::key key2{ 0xa1, 0xa2, 0xa3 };
const table::neutrino::slab slab1{ {}, null_hash, { 0x42 } };
const table::neutrino::slab slab2{ {}, one_hash,  { 0xab, 0xcd, 0xef } };
const data_chunk expected_head = base16_chunk
(
    "0000000000"
    "ffffffffff"
    "2a00000000"
    "ffffffffff"
    "ffffffffff"
    "ffffffffff"
);
const data_chunk closed_head = base16_chunk
(
    "5600000000"
    "ffffffffff"
    "2a00000000"
    "ffffffffff"
    "ffffffffff"
    "ffffffffff"
);
const data_chunk expected_body = base16_chunk
(
    "ffffffffff"
    "010203"     // key1
    "0000000000000000000000000000000000000000000000000000000000000000" // null_hash
    "0142"       // size/bytes

    "0000000000" // next->
    "a1a2a3"     // key2
    "0100000000000000000000000000000000000000000000000000000000000000" // one_hash
    "03abcdef"   // size/bytes
);

BOOST_AUTO_TEST_CASE(neutrino__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::neutrino instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::neutrino::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, key1, slab1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::neutrino::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, key2, slab2));
    BOOST_REQUIRE_EQUAL(link2, 0x2a);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(neutrino__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::neutrino instance{ head_store, body_store, 5 };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::neutrino::slab out{};
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out == slab1);
    BOOST_REQUIRE(instance.get(0x2a, out));
    BOOST_REQUIRE(out == slab2);
}

BOOST_AUTO_TEST_SUITE_END()
