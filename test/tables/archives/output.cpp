/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

BC_AUTO_TEST_SUITE(output_tests)

using namespace system;
const table::output::slab expected
{
    {},                     // schema::output [all const static members]
    0x56341201_u32,         // parent_fk
    0xdebc9a7856341202_u64, // value
    {}                      // script
};
constexpr auto slab0_size = 6u;
const data_chunk expected_file
{
    // slab
    0x00, 0x00, 0x00, 0x00,
    0x00,
    0x00,

    // --------------------------------------------------------------------------------------------

    // slab
    0x01, 0x12, 0x34, 0x56,
    0xff, 0x02, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde,
    0x00
};

BC_AUTO_TEST_CASE(output__put__get__expected)
{
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::output instance{ head_store, body_store };
    BC_REQUIRE(!instance.put_link(table::output::slab{}).is_terminal());
    BC_REQUIRE(!instance.put_link(expected).is_terminal());
    BC_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::output::slab element{};
    BC_REQUIRE(instance.get<table::output::slab>(0, element));
    BC_REQUIRE(element == table::output::slab{});

    BC_REQUIRE(instance.get<table::output::slab>(slab0_size, element));
    BC_REQUIRE(element == expected);
}

BC_AUTO_TEST_SUITE_END()
