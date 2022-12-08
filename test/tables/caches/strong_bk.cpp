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

BOOST_AUTO_TEST_SUITE(strong_bk_tests)

using namespace system;
const table::strong_bk::key key1{ 0x01, 0x02, 0x03 };
const table::strong_bk::key key2{ 0xa1, 0xa2, 0xa3 };
const table::strong_bk::record record1{ {}, 0x42 };
const table::strong_bk::record record2{ {}, 0x24 };
const data_chunk expected_head = base16_chunk
(
    "000000"
    "010000"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
);
const data_chunk closed_head = base16_chunk
(
    "020000"
    "010000"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
);
const data_chunk expected_body = base16_chunk
(
    "ffffff"   // next->end
    "010203"   // key1
    "42"       // code1

    "000000"   // next->
    "a1a2a3"   // key2
    "24"       // code2
);

BOOST_AUTO_TEST_CASE(strong_bk__put__two__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::strong_bk instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::strong_bk::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, key1, record1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::strong_bk::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, key2, record2));
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(strong_bk__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::dfile head_store{ head };
    test::dfile body_store{ body };
    table::strong_bk instance{ head_store, body_store, 5 };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::strong_bk::record out{};
    BOOST_REQUIRE(instance.get(key1, out));
    BOOST_REQUIRE(out == record1);
    BOOST_REQUIRE(instance.get(key2, out));
    BOOST_REQUIRE(out == record2);
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out == record1);
    BOOST_REQUIRE(instance.get(1u, out));
    BOOST_REQUIRE(out == record2);
}

BOOST_AUTO_TEST_SUITE_END()
