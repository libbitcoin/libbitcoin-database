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

BOOST_AUTO_TEST_SUITE(validated_tx_tests)

using namespace system;
const table::validated_tx::key key1{ 0x01, 0x02, 0x03, 0x04 };
const table::validated_tx::key key2{ 0xa1, 0xa2, 0xa3, 0xa4 };
const table::validated_tx::slab in1
{
    {},
    {
        0x11223344, // flags
        0x55667788, // height
        0x99aabbcc  // mtp
    },
    0x42,               // code
    0x1122334455667788, // fee
    0x12345678          // sigops
};
const table::validated_tx::slab in2
{
    {},
    {
        0xaabbccdd, // flags
        0x44332211, // height
        0xabcdef99  // mtp
    },
    0xab,               // code
    0x0000000000000042, // fee
    0x00000055          // sigops
};
const table::validated_tx::slab out1
{
    {},
    {
        0x11223344, // flags
        0x00667788, // height
        0x99aabbcc  // mtp
    },
    0x42,               // code
    0x1122334455667788, // fee
    0x12345678          // sigops
};
const table::validated_tx::slab out2
{
    {},
    {
        0xaabbccdd, // flags
        0x00332211, // height
        0xabcdef99  // mtp
    },
    0xab,               // code
    0x0000000000000042, // fee
    0x00000055          // sigops
};
const data_chunk expected_head = base16_chunk
(
    "0000000000"
    "2300000000"
    "ffffffffff"
    "ffffffffff"
    "ffffffffff"
    "ffffffffff"
);
const data_chunk closed_head = base16_chunk
(
    "3a00000000"
    "2300000000"
    "ffffffffff"
    "ffffffffff"
    "ffffffffff"
    "ffffffffff"
);
const data_chunk expected_body = base16_chunk
(
    "ffffffffff"         // next->end
    "01020304"           // key1
    "44332211"           // flags1
    "887766"             // height1
    "ccbbaa99"           // mtp1
    "42"                 // code1
    "ff8877665544332211" // fees1
    "fe78563412"         // sigops1

    "0000000000"         // next->
    "a1a2a3a4"           // key2
    "ddccbbaa"           // flags2
    "112233"             // height2
    "99efcdab"           // mtp2
    "ab"                 // code2
    "42"                 // sigops2
    "55"                 // fees2
);

BOOST_AUTO_TEST_CASE(validated_tx__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::validated_tx instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::validated_tx::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, key1, in1));
    BOOST_REQUIRE_EQUAL(link1, 0x00u);

    table::validated_tx::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, key2, in2));
    BOOST_REQUIRE_EQUAL(link2, 0x23u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(validated_tx__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::validated_tx instance{ head_store, body_store, 5 };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::validated_tx::slab out{};
    BOOST_REQUIRE(instance.get(0, out));
    BOOST_REQUIRE(out == out1);
    BOOST_REQUIRE(instance.get(0x23, out));
    BOOST_REQUIRE(out == out2);
}

BOOST_AUTO_TEST_SUITE_END()
