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
constexpr hash_digest hash = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
constexpr auto terminal = linkage<record::pk>::terminal;

#define DECLARE(instance_, body_file_, buckets_) \
data_chunk head_file; \
data_chunk body_file_; \
test::storage head_store{ head_file }; \
test::storage body_store{ body_file_ }; \
hashmap<linkage<record::pk>, search<record::sk>, record::size> instance_{ head_store, body_store, buckets_ }

constexpr record expected
{
    1_u32,
    2_u32,
    3_u32,
    terminal,
    4_u32,
    5_u32,
    6_u32,
    7_u32,
    null_hash,
    true
};
const data_chunk expected_file
{
    0xff, 0xff, 0xff,
    0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x01, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff,
    0x04, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

BOOST_AUTO_TEST_CASE(header__put__get__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(hash, expected));
    BOOST_REQUIRE(instance.get<record>(0) == expected);
    BOOST_REQUIRE(instance.get<record>(hash) == expected);
    BOOST_REQUIRE_EQUAL(body_file, expected_file);

    // 000000
    //
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // 000000
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    // ffffff
    //
    // ffffff    [link]
    // 110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f [key]
    // 010000    [height]
    // 02000000  [flags]
    // 03000000  [mtp]
    // ffffff    [parent]
    // 04000000  [version]
    // 05000000  [time]
    // 06000000  [bits]
    // 07000000  [nonce]
    // 0000000000000000000000000000000000000000000000000000000000000000 [root]

    ////std::cout << head_file << std::endl << std::endl;
    ////std::cout << body_file << std::endl << std::endl;
}

BOOST_AUTO_TEST_CASE(header__put__get_minimized__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(hash, expected));

    const auto element = instance.get<record_height>(0);
    BOOST_REQUIRE(element.valid);
    BOOST_REQUIRE_EQUAL(element.height, expected.height);
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_CASE(header__put__get_extended__expected)
{
    DECLARE(instance, body_file, 20);
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(hash, expected));

    const auto element = instance.get<record_with_key>(hash);
    BOOST_REQUIRE(element.valid);
    BOOST_REQUIRE_EQUAL(element.key, hash);
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_SUITE_END()
