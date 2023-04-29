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

BOOST_AUTO_TEST_SUITE(input_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(input__put__empty_twice_get__expected)
{
    const table::input::slab expected
    {
        {}, // schema::input [all const static members]
        {}, // script
        {}  // witness
    };

    const data_chunk expected_file
    {
        // slab0
        0x00, // script
        0x00, // witness

        // slab1
        0x00, // script
        0x00  // witness
    };

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::input instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.put_link(table::input::slab{}).is_terminal());
    BOOST_REQUIRE(!instance.put_link(expected).is_terminal());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::input::slab element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == table::input::slab{});

    BOOST_REQUIRE(instance.get(2, element));
    BOOST_REQUIRE(element == expected);
}

BOOST_AUTO_TEST_CASE(input__put__non_empty_get__expected)
{
    const table::input::slab expected
    {
        {}, // schema::input [all const static members]
        chain::script{ { chain::opcode::checkmultisigverify, chain::opcode::op_return } },
        chain::witness{ { { 0x42 }, { 0x01, 0x02, 0x03 } } }
    };

    const data_chunk expected_file
    {
        // slab0
        0x00, // script
        0x00, // witness

        // slab1
        0x02, 0xaf, 0x6a, // script
        0x02, 0x01, 0x42, 0x03, 0x01, 0x02, 0x03 // witness
    };

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::input instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.put_link(table::input::slab{}).is_terminal());
    BOOST_REQUIRE(!instance.put_link(expected).is_terminal());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::input::slab element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == table::input::slab{});

    BOOST_REQUIRE(instance.get(2u, element));
    BOOST_REQUIRE(element == expected);
}

BOOST_AUTO_TEST_SUITE_END()
