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

BOOST_AUTO_TEST_SUITE(validated_bk_tests)

using namespace system;
const table::validated_bk::record record1{ {}, 0x42 };
const table::validated_bk::record record2{ {}, 0xab };
const auto expected_head = base16_chunk
(
    "000000"
    "000000"
    "010000"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
);
const auto closed_head = base16_chunk
(
    "020000"
    "000000"
    "010000"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
);
const auto expected_body = base16_chunk
(
    "42"                 // code1
    "ab"                 // code2
);

BOOST_AUTO_TEST_CASE(validated_bk__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::validated_bk instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());

    BOOST_REQUIRE(instance.put(0, record1));
    BOOST_REQUIRE_EQUAL(instance.at(0), 0u);
    BOOST_REQUIRE(instance.put(1, record2));
    BOOST_REQUIRE_EQUAL(instance.at(1), 1u);

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

    table::validated_bk::record out{};
    BOOST_REQUIRE(instance.get(0, out));
    BOOST_REQUIRE(out == record1);
    BOOST_REQUIRE(instance.get(1, out));
    BOOST_REQUIRE(out == record2);
}

BOOST_AUTO_TEST_SUITE_END()
