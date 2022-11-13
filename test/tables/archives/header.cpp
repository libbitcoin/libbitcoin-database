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

BOOST_AUTO_TEST_SUITE(tables_archives_header_tests)

using namespace database::header;
constexpr auto terminal = linkage<record::pk>::terminal;
constexpr size_t buckets = 20;

BOOST_AUTO_TEST_CASE(header__put_get__base__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<linkage<record::pk>, search<record::sk>, record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr auto self = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
    BOOST_REQUIRE(instance.put(self, record
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
    }));

    const auto link_record = instance.get<record>(0);
    BOOST_REQUIRE(link_record.valid);
    BOOST_REQUIRE_EQUAL(link_record.height, 1u); // 3 bytes
    BOOST_REQUIRE_EQUAL(link_record.flags, 2u);
    BOOST_REQUIRE_EQUAL(link_record.mtp, 3u);
    BOOST_REQUIRE_EQUAL(link_record.parent_fk, terminal);
    BOOST_REQUIRE_EQUAL(link_record.version, 4u);
    BOOST_REQUIRE_EQUAL(link_record.time, 5u);
    BOOST_REQUIRE_EQUAL(link_record.bits, 6u);
    BOOST_REQUIRE_EQUAL(link_record.nonce, 7u);
    BOOST_REQUIRE_EQUAL(link_record.root, null_hash);

    const auto key_record = instance.get<record>(self);
    BOOST_REQUIRE(key_record.valid);
    BOOST_REQUIRE_EQUAL(key_record.height, 1u); // 3 bytes
    BOOST_REQUIRE_EQUAL(key_record.flags, 2u);
    BOOST_REQUIRE_EQUAL(key_record.mtp, 3u);
    BOOST_REQUIRE_EQUAL(key_record.parent_fk, terminal);
    BOOST_REQUIRE_EQUAL(key_record.version, 4u);
    BOOST_REQUIRE_EQUAL(key_record.time, 5u);
    BOOST_REQUIRE_EQUAL(key_record.bits, 6u);
    BOOST_REQUIRE_EQUAL(key_record.nonce, 7u);
    BOOST_REQUIRE_EQUAL(key_record.root, null_hash);

    const data_chunk expected_file
    {
        0xff, 0xff, 0xff,
        0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x01, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff,
        0x04, 0x00, 0x00, 0x00,
        0x05, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
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

BOOST_AUTO_TEST_CASE(header__put_get__derived__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<linkage<record::pk>, search<record::sk>, record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr auto self = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
    BOOST_REQUIRE(instance.put(self, record
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
    }));

    const auto link_record = instance.get<record_height>(0);
    BOOST_REQUIRE(link_record.valid);
    BOOST_REQUIRE_EQUAL(link_record.height, 1u); // 3 bytes

    const auto key_record = instance.get<record_with_key>(self);
    BOOST_REQUIRE(key_record.valid);
    BOOST_REQUIRE_EQUAL(key_record.key, self);
    BOOST_REQUIRE_EQUAL(key_record.height, 1u); // 3 bytes
    BOOST_REQUIRE_EQUAL(key_record.flags, 2u);
    BOOST_REQUIRE_EQUAL(key_record.mtp, 3u);
    BOOST_REQUIRE_EQUAL(key_record.parent_fk, terminal);
    BOOST_REQUIRE_EQUAL(key_record.version, 4u);
    BOOST_REQUIRE_EQUAL(key_record.time, 5u);
    BOOST_REQUIRE_EQUAL(key_record.bits, 6u);
    BOOST_REQUIRE_EQUAL(key_record.nonce, 7u);
    BOOST_REQUIRE_EQUAL(key_record.root, null_hash);

    const data_chunk expected_file
    {
        0xff, 0xff, 0xff,
        0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x01, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff,
        0x04, 0x00, 0x00, 0x00,
        0x05, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00,
        0x07, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_SUITE_END()
