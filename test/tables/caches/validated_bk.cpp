/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(validated_bk_tests)

using namespace system;
const table::validated_bk::key key1{ 0x01, 0x02, 0x03 };
const table::validated_bk::key key2{ 0xa1, 0xa2, 0xa3 };
const table::validated_bk::slab slab1{ {}, 0x42, 0x1122334455667788 };
const table::validated_bk::slab slab2{ {}, 0xab, 0x0000000000000042 };
const data_chunk expected_head = base16_chunk
(
    "000000"
    "ffffff"
    "100000"
    "ffffff"
    "ffffff"
    "ffffff"
);
const data_chunk closed_head = base16_chunk
(
    "180000"
    "ffffff"
    "100000"
    "ffffff"
    "ffffff"
    "ffffff"
);
const data_chunk expected_body = base16_chunk
(
    "ffffff"  // next->end
    "010203"  // key1
    "42"      // code1
    "ff8877665544332211" // fees1

    "000000"  // next->
    "a1a2a3"  // key2
    "ab"      // code2
    "42"      // fees2
);

BOOST_AUTO_TEST_CASE(validated_bk__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::validated_bk instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::validated_bk::link link1{};
    BOOST_REQUIRE(instance.put_link1(link1, key1, slab1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::validated_bk::link link2{};
    BOOST_REQUIRE(instance.put_link1(link2, key2, slab2));
    BOOST_REQUIRE_EQUAL(link2, 16u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(validated_bk__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::validated_bk instance{ head_store, body_store, 5 };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::validated_bk::slab out{};
    BOOST_REQUIRE(instance.get1(0u, out));
    BOOST_REQUIRE(out == slab1);
    BOOST_REQUIRE(instance.get1(16u, out));
    BOOST_REQUIRE(out == slab2);
}

BOOST_AUTO_TEST_SUITE_END()
