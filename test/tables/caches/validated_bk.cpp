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

BOOST_AUTO_TEST_SUITE(validated_bk_tests)

using namespace system;
const table::validated_bk::slab slab1{ {}, 0x42, 0x1122334455667788 };
const table::validated_bk::slab slab2{ {}, 0xab, 0x0000000000000042 };
const data_chunk expected_head = base16_chunk
(
    "00000000"
    "00000000"
    "0a000000"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
);
const data_chunk closed_head = base16_chunk
(
    "0c000000"
    "00000000"
    "0a000000"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
);
const data_chunk expected_body = base16_chunk
(
    "42"                 // code1
    "ff8877665544332211" // fees1

    "ab"                 // code2
    "42"                 // fees2
);

BOOST_AUTO_TEST_CASE(validated_bk__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::validated_bk instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());

    table::validated_bk::link link1{};
    BOOST_REQUIRE(instance.put(0, slab1));
    BOOST_REQUIRE_EQUAL(instance.at(0), 0u);

    table::validated_bk::link link2{};
    BOOST_REQUIRE(instance.put(1, slab2));
    BOOST_REQUIRE_EQUAL(instance.at(1), 10u);

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
    table::validated_bk instance{ head_store, body_store, 3 };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::validated_bk::slab out{};
    BOOST_REQUIRE(instance.get(0, out));
    BOOST_REQUIRE(out == slab1);
    BOOST_REQUIRE(instance.get(10, out));
    BOOST_REQUIRE(out == slab2);
}

BOOST_AUTO_TEST_SUITE_END()
