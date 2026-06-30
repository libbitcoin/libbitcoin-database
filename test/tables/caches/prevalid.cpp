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
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(prevalid_tests)

using namespace system;
const table::prevalid::record in1{ {}, 0x12345678 };
const table::prevalid::record in2{ {}, 0xabcdef12 };
const table::prevalid::record out1{ {}, 0x00345678 };
const table::prevalid::record out2{ {}, 0x00cdef12 };
const auto expected_head = base16_chunk
(
    "000000"
);
const auto closed_head = base16_chunk
(
    "020000"
);
const auto expected_body = base16_chunk
(
    "785634" // header_fk1
    "12efcd" // header_fk2
);

BOOST_AUTO_TEST_CASE(prevalid__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevalid instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    table::prevalid::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, in1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::prevalid::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, in2));
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(prevalid__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::prevalid instance{ head_store, body_store };
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);

    table::prevalid::record out{};
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out == out1);
    BOOST_REQUIRE(instance.get(1u, out));
    BOOST_REQUIRE(out == out2);
}

BOOST_AUTO_TEST_CASE(prevalid__put_refs__two__expected)
{
    using keys = table::prevalid::put_refs::keys;

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevalid instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    const keys links{ 0x00345678, 0x00cdef12 };
    BOOST_REQUIRE(instance.put(table::prevalid::put_refs{ {}, links }));

    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(prevalid__get_refs__two__expected)
{
    using keys = table::prevalid::get_refs::keys;

    auto head = closed_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::prevalid instance{ head_store, body_store };

    keys links(2u);
    table::prevalid::link first{ 0 };
    table::prevalid::get_refs out{ {}, links };
    BOOST_REQUIRE(instance.get(first, out));
    BOOST_REQUIRE_EQUAL(links.at(0), 0x00345678u);
    BOOST_REQUIRE_EQUAL(links.at(1), 0x00cdef12u);
}

BOOST_AUTO_TEST_CASE(prevalid__put_refs_get_refs__round_trip__expected)
{
    using put_keys = table::prevalid::put_refs::keys;
    using get_keys = table::prevalid::get_refs::keys;

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevalid instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    const put_keys in{ 0x00345678, 0x00cdef12 };
    const table::prevalid::put_refs writer{ {}, in };
    BOOST_REQUIRE(instance.put(writer));

    get_keys result(in.size());
    table::prevalid::link first{ 0 };
    table::prevalid::get_refs reader{ {}, result };
    BOOST_REQUIRE(instance.get(first, reader));
    BOOST_REQUIRE(result == in);
}

BOOST_AUTO_TEST_SUITE_END()
