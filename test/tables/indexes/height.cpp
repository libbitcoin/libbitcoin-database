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

BOOST_AUTO_TEST_SUITE(height_tests)

using namespace system;
const table::height::record in1{ {}, 0x12345678 };
const table::height::record in2{ {}, 0xabcdef12 };
const table::height::record out1{ {}, 0x00345678 };
const table::height::record out2{ {}, 0x00cdef12 };
const data_chunk expected_head = base16_chunk
(
    "000000"
);
const data_chunk closed_head = base16_chunk
(
    "020000"
);
const data_chunk expected_body = base16_chunk
(
    "785634" // header_fk1
    "12efcd" // header_fk2
);

BOOST_AUTO_TEST_CASE(height__put__two__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::height instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    table::height::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, in1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::height::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, in2));
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(height__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::dfile head_store{ head };
    test::dfile body_store{ body };
    table::height instance{ head_store, body_store };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::height::record out{};
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out == out1);
    BOOST_REQUIRE(instance.get(1u, out));
    BOOST_REQUIRE(out == out2);
}

BOOST_AUTO_TEST_SUITE_END()
