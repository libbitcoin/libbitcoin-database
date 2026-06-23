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

BOOST_AUTO_TEST_SUITE(silent_tests)

using namespace system;

const table::silent::record record1
{
    {},
    0x1122334455667788_u64, // prefix
    base16_array("222222222222222222222222222222222222222222222222222222222222222222"),
    0xabcdef12_u32          // tx_fk
};

const table::silent::record record2
{
    {},
    0xabcdef0123456789_u64, // prefix
    base16_array("444444444444444444444444444444444444444444444444444444444444444444"),
    0x12345678_u32          // tx_fk
};

const auto expected_head = base16_chunk("00000000");
const auto closed_head   = base16_chunk("02000000");
const auto expected_body = base16_chunk
(
    // record 1
    "8877665544332211"
    "222222222222222222222222222222222222222222222222222222222222222222"
    "12efcdab"

    // record 2
    "8967452301efcdab"
    "444444444444444444444444444444444444444444444444444444444444444444"
    "78563412"
);

BOOST_AUTO_TEST_CASE(silent__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::silent instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    table::silent::link link1{};
    BOOST_REQUIRE(instance.put_link(link1, record1));
    BOOST_REQUIRE_EQUAL(link1, 0u);

    table::silent::link link2{};
    BOOST_REQUIRE(instance.put_link(link2, record2));
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BOOST_AUTO_TEST_CASE(silent__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::silent instance{ head_store, body_store };

    table::silent::record out{};
    BOOST_REQUIRE(instance.get(0u, out));
    BOOST_REQUIRE(out == record1);
    BOOST_REQUIRE(instance.get(1u, out));
    BOOST_REQUIRE(out == record2);
}

BOOST_AUTO_TEST_SUITE_END()
