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

BOOST_AUTO_TEST_SUITE(header_tests)

using namespace system;
constexpr hash_digest key = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
constexpr hash_digest merkle_root = base16_array("330102030405060708090a0b0c0d0e0f440102030405060708090a0b0c0d0e0f");
constexpr table::header::record expected
{
    {}, // schema::header [all const static members]
    context
    {
        0x00341201_u32, // height
        0x56341202_u32, // flags
        0x56341203_u32  // mtp
    },
    0x00341204_u32, // parent_fk
    0x56341205_u32, // version
    merkle_root,
    0x56341206_u32, // timestamp
    0x56341207_u32, // bits
    0x56341208_u32  // nonce
};
const system::chain::header expected_header
{
    expected.version,
    hash_digest{}, // parent (unused)
    expected.merkle_root,
    expected.timestamp,
    expected.bits,
    expected.nonce
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
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

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
    0x33, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x44, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x06, 0x12, 0x34, 0x56,
    0x07, 0x12, 0x34, 0x56,
    0x08, 0x12, 0x34, 0x56
};

BOOST_AUTO_TEST_CASE(header__put__get__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::header instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::header::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::header::record element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == table::header::record{});

    BOOST_REQUIRE(instance.get(null_hash, element));
    BOOST_REQUIRE(element == table::header::record{});

    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE(element == expected);

    BOOST_REQUIRE(instance.get(key, element));
    BOOST_REQUIRE(element == expected);
}

BOOST_AUTO_TEST_CASE(header__put_ptr__get_ptr__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::header instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::header::record{}));

    const table::header::record_put_ptr put_ptr
    {
        {},
        expected.context,
        expected.parent_fk,
        system::to_shared(expected_header)
    };
    BOOST_REQUIRE(instance.put(key, put_ptr));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::header::record_get_ptr get_ptr{};
    BOOST_REQUIRE(instance.get(1, get_ptr));
    BOOST_REQUIRE(get_ptr.context == put_ptr.context);
    BOOST_REQUIRE(get_ptr.header_ptr);
    BOOST_REQUIRE(*get_ptr.header_ptr == *put_ptr.header_ptr);
    BOOST_REQUIRE_EQUAL(get_ptr.parent_fk, put_ptr.parent_fk);
}

BOOST_AUTO_TEST_CASE(header__put__get_with_sk__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::header instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::header::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::header::record_with_sk element{};
    BOOST_REQUIRE(instance.get<table::header::record_with_sk>(key, element));
    BOOST_REQUIRE(static_cast<table::header::record>(element) == expected);
    BOOST_REQUIRE_EQUAL(element.key, key);
}

BOOST_AUTO_TEST_CASE(point__put__get_sk__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::header instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::header::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::header::record_sk element{};
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.key, key);
}

BOOST_AUTO_TEST_CASE(header__put__get_height__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::header instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::header::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    table::header::record_height element{};
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.height, expected.context.height);
}

BOOST_AUTO_TEST_CASE(point__it__pk__expected)
{
    test::dfile head_store{};
    test::dfile body_store{};
    table::header instance{ head_store, body_store, 20 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::header::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);

    auto it = instance.it(key);
    BOOST_REQUIRE_EQUAL(it.self(), 1u);
    BOOST_REQUIRE(!it.advance());
}

BOOST_AUTO_TEST_SUITE_END()
