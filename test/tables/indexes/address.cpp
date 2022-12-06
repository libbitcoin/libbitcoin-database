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
#include "../../mocks/dfile.hpp"

BOOST_AUTO_TEST_SUITE(address_tests)

using namespace system;
const table::address::key key1{ 0x01, 0x02, 0x03, 0x04 };
const table::address::key key2{ 0xa1, 0xa2, 0xa3, 0xa4 };
const table::address::record in1{ {}, 0x1234567890abcdef };
const table::address::record in2{ {}, 0xabcdef1234567890 };
const table::address::record out1{ {}, 0x0000007890abcdef };
const table::address::record out2{ {}, 0x0000001234567890 };
const data_chunk expected_head = base16_chunk
(
    "00000000"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "01000000"
);
const data_chunk closed_head = base16_chunk
(
    "02000000"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "01000000"
);
const data_chunk expected_body = base16_chunk
(
    "ffffffff"   // next->end
    "01020304"   // key1
    "efcdab9078" // output1 [low 5 bytes]

    "00000000"   // next->
    "a1a2a3a4"   // key2
    "9078563412" // output2 [low 5 bytes]
);

BOOST_AUTO_TEST_CASE(address__put__two__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::address instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::address::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, key1, in1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::address::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, key2, in2));
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(address__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::dfile head_store{ head };
    test::dfile body_store{ body };
    table::address instance{ head_store, body_store, 5 };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::address::record out{};
    BOOST_REQUIRE(instance.get(key1, out));
    BOOST_REQUIRE(out == out1);
    BOOST_REQUIRE(instance.get(key2, out));
    BOOST_REQUIRE(out == out2);
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out == out1);
    BOOST_REQUIRE(instance.get(1u, out));
    BOOST_REQUIRE(out == out2);
}

BOOST_AUTO_TEST_SUITE_END()
