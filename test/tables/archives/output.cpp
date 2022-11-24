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
#include "../../storage.hpp"

BOOST_AUTO_TEST_SUITE(output_tests)

using namespace system;

#define DECLARE(instance_, body_file_) \
data_chunk head_file; \
data_chunk body_file_; \
test::storage head_store{ head_file }; \
test::storage body_store{ body_file_ }; \
table::output instance_{ head_store, body_store }

const table::output::slab expected
{
    {},                     // schema::output [all const static members]
    0x56341201_u32,         // parent_fk
    0x00000042_u32,         // index
    0xdebc9a7856341202_u64, // value
    {}                      // script
};
constexpr auto slab0_size = 7u;
const data_chunk expected_file
{
    // slab
    0x00, 0x00, 0x00, 0x00,
    0x00,
    0x00,
    0x00,

    // --------------------------------------------------------------------------------------------

    // slab
    0x01, 0x12, 0x34, 0x56,
    0x42,
    0xff, 0x02, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde,
    0x00
};

BOOST_AUTO_TEST_CASE(output__put__get__expected)
{
    DECLARE(instance, body_file);
    BOOST_REQUIRE(instance.put(table::output::slab{}));
    BOOST_REQUIRE(instance.put(expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    table::output::slab element{};
    BOOST_REQUIRE(instance.get<table::output::slab>(0, element));
    BOOST_REQUIRE(element == table::output::slab{});

    BOOST_REQUIRE(instance.get<table::output::slab>(slab0_size, element));
    BOOST_REQUIRE(element == expected);
}

BOOST_AUTO_TEST_SUITE_END()
