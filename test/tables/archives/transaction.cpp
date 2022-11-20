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

BOOST_AUTO_TEST_SUITE(transaction_tests)

using namespace system;
constexpr hash_digest key = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");

#define DECLARE(instance_, body_file_, buckets_) \
data_chunk head_file; \
data_chunk body_file_; \
test::storage head_store{ head_file }; \
test::storage body_store{ body_file_ }; \
table::transaction instance_{ head_store, body_store, buckets_ }

constexpr table::transaction::record expected
{
    {},             // schema::output [all const static members]
    true,           // coinbase
    0x00341201_u32, // bytes
    0x00341202_u32, // weight
    0x56341203_u32, // locktime
    0x56341204_u32, // version
    0x00341205_u32, // ins_count
    0x00341206_u32, // outs_count
    0x56341207_u32  // puts_fk (ins_fk)
};
const data_chunk expected_file
{
    // next
    0xff, 0xff, 0xff, 0xff,

    // key
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // record
    0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    // --------------------------------------------------------------------------------------------

    // next
    0xff, 0xff, 0xff, 0xff,

    // key
    0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,

    // record
    0x01,
    0x01, 0x12, 0x34,
    0x02, 0x12, 0x34,
    0x03, 0x12, 0x34, 0x56,
    0x04, 0x12, 0x34, 0x56,
    0x05, 0x12, 0x34,
    0x06, 0x12, 0x34,
    0x07, 0x12, 0x34, 0x56
};

BOOST_AUTO_TEST_CASE(transaction__put__get__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::transaction::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    table::transaction::record element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == table::transaction::record{});

    BOOST_REQUIRE(instance.get(null_hash, element));
    BOOST_REQUIRE(element == table::transaction::record{});

    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE(element == expected);

    BOOST_REQUIRE(instance.get(key, element));
    BOOST_REQUIRE(element == expected);

    BOOST_REQUIRE(!is_multiply_overflow<size_t>(element.ins_count, schema::put));
    BOOST_REQUIRE(!is_add_overflow<size_t>(element.ins_fk, element.ins_count * schema::put));
    BOOST_REQUIRE_EQUAL(element.outs_fk(), element.ins_fk + element.ins_count * schema::put);
}

BOOST_AUTO_TEST_CASE(transaction__put__get_sk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::transaction::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    table::transaction::record_sk element{};
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.key, key);
}

BOOST_AUTO_TEST_CASE(transaction__put__get_puts__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::transaction::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    table::transaction::record_puts element{};
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.ins_count, 0x00341205_u32);
    BOOST_REQUIRE_EQUAL(element.outs_count, 0x00341206_u32);
    BOOST_REQUIRE_EQUAL(element.ins_fk, 0x56341207_u32);
    BOOST_REQUIRE(!is_multiply_overflow<size_t>(element.ins_count, schema::put));
    BOOST_REQUIRE(!is_add_overflow<size_t>(element.ins_fk, element.ins_count * schema::put));
    BOOST_REQUIRE_EQUAL(element.outs_fk(), element.ins_fk + element.ins_count * schema::put);
}

BOOST_AUTO_TEST_CASE(transaction__it__pk__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put({}, table::transaction::record{}));
    BOOST_REQUIRE(instance.put(key, expected));
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    auto it = instance.it(key);
    BOOST_REQUIRE_EQUAL(it.self(), 1u);
    BOOST_REQUIRE(!it.advance());
}

BOOST_AUTO_TEST_SUITE_END()
