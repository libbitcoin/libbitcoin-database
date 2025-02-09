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

BOOST_AUTO_TEST_SUITE(point_tests)

using namespace system;
constexpr auto hash = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
const data_chunk expected_file
{
    // null_hash
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // --------------------------------------------------------------------------------------------

    // hash
    0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};

BOOST_AUTO_TEST_CASE(point__put__get__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::point instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());

    table::point::link link{};
    BOOST_REQUIRE(instance.put_link(link, table::point::record{}));
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE(instance.put_link(link, table::point::record{ {}, hash }));
    BOOST_REQUIRE_EQUAL(link, 1u);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::point::record element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE_EQUAL(element.hash, null_hash);
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.hash, hash);
}

BOOST_AUTO_TEST_SUITE_END()
