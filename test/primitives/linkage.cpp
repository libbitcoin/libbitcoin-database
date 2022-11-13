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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(linkage_tests)

// Minimal serialized sizes.
static_assert(linkage<0>::size == 0);
static_assert(linkage<1>::size == 1);
static_assert(linkage<2>::size == 2);
static_assert(linkage<3>::size == 3);
static_assert(linkage<4>::size == 4);
static_assert(linkage<5>::size == 5);
static_assert(linkage<6>::size == 6);
static_assert(linkage<7>::size == 7);
static_assert(linkage<8>::size == 8);

// Minimal serialized representations.
static_assert(is_same_type<linkage<0>::bytes, data_array<0>>);
static_assert(is_same_type<linkage<1>::bytes, data_array<1>>);
static_assert(is_same_type<linkage<2>::bytes, data_array<2>>);
static_assert(is_same_type<linkage<3>::bytes, data_array<3>>);
static_assert(is_same_type<linkage<4>::bytes, data_array<4>>);
static_assert(is_same_type<linkage<5>::bytes, data_array<5>>);
static_assert(is_same_type<linkage<6>::bytes, data_array<6>>);
static_assert(is_same_type<linkage<7>::bytes, data_array<7>>);
static_assert(is_same_type<linkage<8>::bytes, data_array<8>>);

// Minimal integer representations.
static_assert(is_same_type<linkage<0>::integer, size_t>);
static_assert(is_same_type<linkage<1>::integer, uint8_t>);
static_assert(is_same_type<linkage<2>::integer, uint16_t>);
static_assert(is_same_type<linkage<3>::integer, uint32_t>);
static_assert(is_same_type<linkage<4>::integer, uint32_t>);
static_assert(is_same_type<linkage<5>::integer, uint64_t>);
static_assert(is_same_type<linkage<6>::integer, uint64_t>);
static_assert(is_same_type<linkage<7>::integer, uint64_t>);
static_assert(is_same_type<linkage<8>::integer, uint64_t>);

// Expected terminal representations.
static_assert(linkage<0>::terminal == 0u);
static_assert(linkage<1>::terminal == mask_left<uint8_t>(0));
static_assert(linkage<2>::terminal == mask_left<uint16_t>(0));
static_assert(linkage<3>::terminal == mask_left<uint32_t>(8));
static_assert(linkage<4>::terminal == mask_left<uint32_t>(0));
static_assert(linkage<5>::terminal == mask_left<uint64_t>(24));
static_assert(linkage<6>::terminal == mask_left<uint64_t>(16));
static_assert(linkage<7>::terminal == mask_left<uint64_t>(8));
static_assert(linkage<8>::terminal == mask_left<uint64_t>(0));

// is_terminal() method.
static_assert(linkage<0>{ 0x00 }.is_terminal());
static_assert(linkage<1>{ 0xff }.is_terminal());
static_assert(linkage<2>{ 0xffff }.is_terminal());
static_assert(linkage<3>{ 0x00ffffff }.is_terminal());
static_assert(linkage<4>{ 0xffffffff }.is_terminal());
static_assert(linkage<5>{ 0x000000ffffffffff }.is_terminal());
static_assert(linkage<6>{ 0x0000ffffffffffff }.is_terminal());
static_assert(linkage<7>{ 0x00ffffffffffffff }.is_terminal());
static_assert(linkage<8>{ 0xffffffffffffffff }.is_terminal());

// Default constructions (integer() cast operator for equality test).
static_assert(linkage<0>{} == linkage<0>::terminal);
static_assert(linkage<1>{} == linkage<1>::terminal);
static_assert(linkage<2>{} == linkage<2>::terminal);
static_assert(linkage<3>{} == linkage<3>::terminal);
static_assert(linkage<4>{} == linkage<4>::terminal);
static_assert(linkage<5>{} == linkage<5>::terminal);
static_assert(linkage<6>{} == linkage<6>::terminal);
static_assert(linkage<7>{} == linkage<7>::terminal);
static_assert(linkage<8>{} == linkage<8>::terminal);

// Non-default constructions (integer() cast operator for equality test).
static_assert(linkage<0>{ 42 } == 42_size);
static_assert(linkage<1>{ 42 } == 42_u8);
static_assert(linkage<2>{ 42 } == 42_u16);
static_assert(linkage<3>{ 42 } == 42_u32);
static_assert(linkage<4>{ 42 } == 42_u32);
static_assert(linkage<5>{ 42 } == 42_u64);
static_assert(linkage<6>{ 42 } == 42_u64);
static_assert(linkage<7>{ 42 } == 42_u64);
static_assert(linkage<8>{ 42 } == 42_u64);

// assign integer

BOOST_AUTO_TEST_CASE(linkage__assign_integer__1__expected)
{
	constexpr auto expected = max_uint8;
	linkage<1> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__2__expected)
{
	constexpr auto expected = max_uint16;
	linkage<2> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__3__expected)
{
	constexpr auto expected = sub1(system::power2(3u * 8u));
	linkage<3> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__4__expected)
{
	constexpr auto expected = max_uint32;
	linkage<4> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__5__expected)
{
	constexpr auto expected = sub1(system::power2<uint64_t>(5u * 8u));
	linkage<5> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__6__expected)
{
	constexpr auto expected = sub1(system::power2<uint64_t>(6u * 8u));
	linkage<6> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__7__expected)
{
	constexpr auto expected = sub1(system::power2<uint64_t>(7u * 8u));
	linkage<7> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_integer__8__expected)
{
	constexpr auto expected = max_uint64;
	linkage<8> instance{};
	instance = expected;
	BOOST_REQUIRE_EQUAL(instance, expected);
}

// assign bytes

BOOST_AUTO_TEST_CASE(linkage__assign_array__0__expected)
{
	constexpr auto expected = 0_size;
	linkage<0> instance{ 0x00 };
	instance = data_array<0>{};
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__1__expected)
{
	constexpr auto expected = 0x01_u8;
	linkage<1> instance{};
	instance = data_array<1>{ 0x01 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__2__expected)
{
	constexpr auto expected = 0x0102_u16;
	linkage<2> instance{};
	instance = data_array<2>{ 0x02, 0x01 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__3__expected)
{
	constexpr auto expected = 0x00020304_u32;
	linkage<3> instance{};
	instance = data_array<3>{ 0x04, 0x03, 0x02 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__4__expected)
{
	constexpr auto expected = 0x01020304_u32;
	linkage<4> instance{};
	instance = data_array<4>{ 0x04, 0x03, 0x02, 0x01 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__5__expected)
{
	constexpr auto expected = 0x0000000405060708_u64;
	linkage<5> instance{};
	instance = data_array<5>{ 0x08, 0x07, 0x06, 0x05, 0x04 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__6__expected)
{
	constexpr auto expected = 0x0000030405060708_u64;
	linkage<6> instance{};
	instance = data_array<6>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__7__expected)
{
	constexpr auto expected = 0x0002030405060708_u64;
	linkage<7> instance{};
	instance = data_array<7>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__assign_array__8__expected)
{
	// LE
	constexpr auto expected = 0x0102030405060708_u64;
	linkage<8> instance{};
	instance = data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

// cast bytes

BOOST_AUTO_TEST_CASE(linkage__bytes__0__expected)
{
	constexpr auto expected = data_array<0>{};
	constexpr linkage<0> instance{ 0x00_size };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<0>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__1__expected)
{
	constexpr auto expected = data_array<1>{ 0x01 };
	constexpr linkage<1> instance{ 0x01_u8 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<1>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__2__expected)
{
	constexpr auto expected = data_array<2>{ 0x02, 0x01 };
	constexpr linkage<2> instance{ 0x0102_u16 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<2>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__3__expected)
{
	constexpr auto expected = data_array<3>{ 0x04, 0x03, 0x02 };
	constexpr linkage<3> instance{ 0x01020304_u32 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<3>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__4__expected)
{
	constexpr auto expected = data_array<4>{ 0x04, 0x03, 0x02, 0x01 };
	constexpr linkage<4> instance{ 0x01020304_u32 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<4>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__5__expected)
{
	constexpr auto expected = data_array<5>{ 0x08, 0x07, 0x06, 0x05, 0x04 };
	constexpr linkage<5> instance{ 0x0102030405060708_u64 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<5>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__6__expected)
{
	constexpr auto expected = data_array<6>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03 };
	constexpr linkage<6> instance{ 0x0102030405060708_u64 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<6>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__7__expected)
{
	constexpr auto expected = data_array<7>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02 };
	constexpr linkage<7> instance{ 0x0102030405060708_u64 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<7>&>(instance), expected);
}

BOOST_AUTO_TEST_CASE(linkage__bytes__8__expected)
{
	constexpr auto expected = data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
	constexpr linkage<8> instance{ 0x0102030405060708_u64 };
	BOOST_REQUIRE_EQUAL(static_cast<const data_array<8>&>(instance), expected);
}

// construct bytes

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__1__expected)
{
	constexpr auto expected = 0x01_u8;
	const linkage<1> instance{ data_array<1>{ 0x01 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__2__expected)
{
	constexpr auto expected = 0x0102_u16;
	const linkage<2> instance{ data_array<2>{ 0x02, 0x01 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__3__expected)
{
	constexpr auto expected = 0x00020304_u32;
	const linkage<3> instance{ data_array<3>{ 0x04, 0x03, 0x02 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__4__expected)
{
	constexpr auto expected = 0x01020304_u32;
	const linkage<4> instance{ data_array<4>{ 0x04, 0x03, 0x02, 0x01 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__5__expected)
{
	constexpr auto expected = 0x0000000405060708_u64;
	const linkage<5> instance{ data_array<5>{ 0x08, 0x07, 0x06, 0x05, 0x04 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__6__expected)
{
	constexpr auto expected = 0x0000030405060708_u64;
	const linkage<6> instance{ data_array<6>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__7__expected)
{
	constexpr auto expected = 0x0002030405060708_u64;
	const linkage<7> instance{ data_array<7>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__construct_bytes__8__expected)
{
	constexpr auto expected = 0x0102030405060708_u64;
	const linkage<8> instance{ data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 } };
	BOOST_REQUIRE_EQUAL(instance, expected);
}

BOOST_AUTO_TEST_CASE(linkage__equality__equal__expected)
{
	const linkage<8> left{ data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 } };
	const linkage<8> right{ data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 } };
	BOOST_REQUIRE(left == right);
	BOOST_REQUIRE(!(left != right));
}

BOOST_AUTO_TEST_CASE(linkage__equality__not_equal__expected)
{
	const linkage<8> left{ data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x00 } };
	const linkage<8> right{ data_array<8>{ 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 } };
	BOOST_REQUIRE(left != right);
	BOOST_REQUIRE(!(left == right));
}

BOOST_AUTO_TEST_SUITE_END()
