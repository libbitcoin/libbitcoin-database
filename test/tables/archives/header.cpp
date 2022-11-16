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

BOOST_AUTO_TEST_SUITE(header_tests)

using namespace database::header;
constexpr hash_digest key = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
constexpr hash_digest root = base16_array("330102030405060708090a0b0c0d0e0f440102030405060708090a0b0c0d0e0f");

#define DECLARE(instance_, body_file_, buckets_) \
data_chunk head_file; \
data_chunk body_file_; \
test::storage head_store{ head_file }; \
test::storage body_store{ body_file_ }; \
hash_map<record> instance_{ head_store, body_store, buckets_ }

constexpr record expected
{
    0x00341201_u32, // height
    0x56341202_u32, // flags
    0x56341203_u32, // mtp
    0x00341204_u32, // parent_fk
    0x56341205_u32, // version
    0x56341206_u32, // time
    0x56341207_u32, // bits
    0x56341208_u32, // nonce
    root
};
const data_chunk expected_file
{
    // next
    0xff, 0xff, 0xff,

    // key
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // record
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // --------------------------------------------------------------------------------------------

    // next
    0xff, 0xff, 0xff,

    // key
    0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,

    // record
    0x01, 0x12, 0x34,
    0x02, 0x12, 0x34, 0x56,
    0x03, 0x12, 0x34, 0x56,
    0x04, 0x12, 0x34,
    0x05, 0x12, 0x34, 0x56,
    0x06, 0x12, 0x34, 0x56,
    0x07, 0x12, 0x34, 0x56,
    0x08, 0x12, 0x34, 0x56,
    0x33, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x44, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

BOOST_AUTO_TEST_CASE(header__put__get__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    record element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == record{});

    BOOST_REQUIRE(instance.get(null_hash, element));
    BOOST_REQUIRE(element == record{});

    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE(element == expected);

    BOOST_REQUIRE(instance.get(key, element));
    BOOST_REQUIRE(element == expected);
}

BOOST_AUTO_TEST_CASE(point__put__get_sk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    record_sk element{};
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.sk, key);
}

BOOST_AUTO_TEST_CASE(point__it__pk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    auto it = instance.it(key);
    BOOST_REQUIRE_EQUAL(it.self(), 1u);
    BOOST_REQUIRE(!it.advance());
}

BOOST_AUTO_TEST_CASE(header__put__get_height__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    record_height element{};
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.height, expected.height);
}

BOOST_AUTO_TEST_CASE(header__put__get_with_sk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    record_with_sk element{};
    BOOST_REQUIRE(instance.get<record_with_sk>(key, element));
    BOOST_REQUIRE(static_cast<record>(element) == expected);
    BOOST_REQUIRE_EQUAL(element.sk, key);
}

BOOST_AUTO_TEST_SUITE_END()
