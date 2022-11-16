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

BOOST_AUTO_TEST_SUITE(input_tests)

using namespace database::input;
constexpr search<slab::sk> key = base16_array("11223344556677");

#define DECLARE(instance_, body_file_, buckets_) \
data_chunk head_file; \
data_chunk body_file_; \
test::storage head_store{ head_file }; \
test::storage body_store{ body_file_ }; \
hash_map<slab> instance_{ head_store, body_store, buckets_ }

const slab expected
{
    0x56341201_u32, // parent_fk
    0x00000000_u32, // index
    0x56341202_u32, // sequence
    {},             // script
    {}              // witness
};
constexpr auto slab0_size = 23u;
const data_chunk expected_file
{
    // next
    0xff, 0xff, 0xff, 0xff, 0xff,

    // key
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // slab
    0x00, 0x00, 0x00, 0x00,
    0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00,
    0x00,

    // --------------------------------------------------------------------------------------------

    // next
    0xff, 0xff, 0xff, 0xff, 0xff,

    // key
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,

    // slab
    0x01, 0x12, 0x34, 0x56,
    0x00,
    0x02, 0x12, 0x34, 0x56,
    0x00,
    0x00
};

BOOST_AUTO_TEST_CASE(input__put__get__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, slab{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    slab element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == slab{});

    BOOST_REQUIRE(instance.get(search<slab::sk>{}, element));
    BOOST_REQUIRE(element == slab{});

    BOOST_REQUIRE(instance.get(slab0_size, element));
    BOOST_REQUIRE(element == expected);

    BOOST_REQUIRE(instance.get(key, element));
    BOOST_REQUIRE(element == expected);
}

BOOST_AUTO_TEST_CASE(input__put__get_sk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, slab{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    slab_sk element{};
    BOOST_REQUIRE(instance.get(slab0_size, element));
    BOOST_REQUIRE_EQUAL(element.sk, key);
}

BOOST_AUTO_TEST_CASE(input__it__pk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, slab{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    auto it = instance.it(key);
    BOOST_REQUIRE_EQUAL(it.self(), slab0_size);
    BOOST_REQUIRE(!it.advance());
}

BOOST_AUTO_TEST_SUITE_END()
