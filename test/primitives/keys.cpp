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
#include "../test.hpp"

#include <bitset>

using namespace system;
using namespace database;

BOOST_AUTO_TEST_SUITE(keys_tests)

constexpr auto hash0 = base16_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
constexpr auto hash1 = base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
constexpr auto hash2 = base16_hash("bf7c3f5a69a78edd81f3eff7e93a37fb2d7da394d48db4d85e7e5353b9b8e270");

BOOST_AUTO_TEST_CASE(keys__hash__points__expected)
{
    const chain::point instance0{ hash0, 0x01234567_u32 };
    const auto hash0 = keys::hash(instance0);
    BOOST_REQUIRE_EQUAL(hash0, static_cast<size_t>(0xe567e36c143a81b9_u64));

    const chain::point instance1{ hash1, 0x12345678_u32 };
    const auto hash1 = keys::hash(instance1);
    BOOST_REQUIRE_EQUAL(hash1, static_cast<size_t>(0x6424f6f5e9ef100e_u64));

    const chain::point instance2{ hash2, 0x23456789_u32 };
    const auto hash2 = keys::hash(instance2);
    BOOST_REQUIRE_EQUAL(hash2, static_cast<size_t>(0xbcfca6a75034a369_u64));
}

BOOST_AUTO_TEST_CASE(keys__thumb__points__expected)
{
    const chain::point instance0{ hash0, 0x01234567_u32 };
    const auto hash0 = keys::thumb(instance0);
    BOOST_REQUIRE_EQUAL(hash0, static_cast<size_t>(0x68d6190000000000_u64));

    const chain::point instance1{ hash1, 0x12345678_u32 };
    const auto hash1 = keys::thumb(instance1);
    BOOST_REQUIRE_EQUAL(hash1, static_cast<size_t>(0x3a9fb8aa4b1e5e4a_u64));

    const chain::point instance2{ hash2, 0x23456789_u32 };
    const auto hash2 = keys::thumb(instance2);
    BOOST_REQUIRE_EQUAL(hash2, static_cast<size_t>(0xdd8ea7695a3f7cbf_u64));
}

////BOOST_AUTO_TEST_CASE(keys__hash_thumb__similarity__expected)
////{
////    const chain::point instance0{ hash0, 0x01234567_u32 };
////    const chain::point instance1{ hash1, 0x12345678_u32 };
////    const chain::point instance2{ hash2, 0x23456789_u32 };
////    const auto xor0 = bit_xor(keys::hash(instance0), keys::thumb(instance0));
////    const auto xor1 = bit_xor(keys::hash(instance1), keys::thumb(instance1));
////    const auto xor2 = bit_xor(keys::hash(instance2), keys::thumb(instance2));
////
////    std::cout << std::endl;
////    std::cout << std::bitset<bits<size_t>>(keys::hash(instance0)) << std::endl;
////    std::cout << std::bitset<bits<size_t>>(keys::thumb(instance0)) << std::endl;
////    std::cout << std::bitset<bits<size_t>>(xor0) << std::endl;
////    std::cout << system::ones_count(xor0);
////
////    std::cout << std::endl;
////    std::cout << std::bitset<bits<size_t>>(keys::hash(instance1)) << std::endl;
////    std::cout << std::bitset<bits<size_t>>(keys::thumb(instance1)) << std::endl;
////    std::cout << std::bitset<bits<size_t>>(xor1) << std::endl;
////    std::cout << system::ones_count(xor1);
////
////    std::cout << std::endl;
////    std::cout << std::bitset<bits<size_t>>(keys::hash(instance2)) << std::endl;
////    std::cout << std::bitset<bits<size_t>>(keys::thumb(instance2)) << std::endl;
////    std::cout << std::bitset<bits<size_t>>(xor2) << std::endl;
////    std::cout << system::ones_count(xor2);
////}

BOOST_AUTO_TEST_SUITE_END()
