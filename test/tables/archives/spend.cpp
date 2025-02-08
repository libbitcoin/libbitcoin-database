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

BOOST_AUTO_TEST_SUITE(spend_tests)

using namespace system;

using namespace system;
const table::spend::key key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
const table::spend::key key2{ 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7 };
const table::spend::record in1{ {}, 0xbaadf00d, 0xaabbccdd, 1, 2 };
const table::spend::record in2{ {}, 0xdeadbeef, 0x11223344, 3, 4 };
const data_chunk expected_head = base16_chunk
(
    "00000000"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "01000000"
    "ffffffff"
);
const data_chunk closed_head = base16_chunk
(
    "02000000"
    "ffffffff"
    "ffffffff"
    "ffffffff"
    "01000000"
    "ffffffff"
);
const data_chunk expected_body = base16_chunk
(
    "ffffffff"       // next->end
    "01020304050607" // key1
    "0df0adba"       // point_fk1
    "ddccbbaa"       // parent_fk1
    "01000000"       // sequence1
    "0200000000"     // input_fk1

    "00000000"       // next->
    "a1a2a3a4a5a6a7" // key2
    "efbeadde"       // point_fk2
    "44332211"       // parent_fk2
    "03000000"       // sequence2
    "0400000000"     // input_fk2
);

BOOST_AUTO_TEST_CASE(spend__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::spend instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::spend::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, key1, in1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::spend::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, key2, in2));
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_SUITE_END()
