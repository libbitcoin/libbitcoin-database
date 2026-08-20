/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(headmap_tests)

using namespace system;

using link = linkage<2>;
using test_headmap = headmap<link, sizeof(uint16_t)>;
using small_link = linkage<3>;
using unaligned_headmap = headmap<small_link, small_link::size>;

BOOST_AUTO_TEST_CASE(headmap__create__empty__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(instance.head_size(), zero);
    BOOST_REQUIRE_EQUAL(instance.body_size(), zero);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(instance.at(0).is_terminal());
}

BOOST_AUTO_TEST_CASE(headmap__push__reserved__expected_at)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.push(link::terminal));
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x1122_u16));
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x3344_u16));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(0), 0x1122_u16);
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x3344_u16);
    BOOST_REQUIRE(instance.at(2).is_terminal());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("22114433"));
}

BOOST_AUTO_TEST_CASE(headmap__push__unreserved__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.push(0x1122_u16));
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

BOOST_AUTO_TEST_CASE(headmap__truncate__push__rewrites_bucket)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.reserve(2u));
    BOOST_REQUIRE(instance.push(0x1122_u16));
    BOOST_REQUIRE(instance.push(0x3344_u16));
    BOOST_REQUIRE(!instance.truncate(3u));
    BOOST_REQUIRE(instance.truncate(1u));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(instance.at(1).is_terminal());
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x5566_u16));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x5566_u16);
}

BOOST_AUTO_TEST_CASE(headmap__unaligned__push_at_truncate__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    unaligned_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.reserve(2u));
    BOOST_REQUIRE(instance.push(0x00112233_u32));
    BOOST_REQUIRE(instance.push(0x00445566_u32));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(0), 0x00112233_u32);
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x00445566_u32);
    BOOST_REQUIRE(instance.at(2).is_terminal());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("332211665544"));
    BOOST_REQUIRE(instance.truncate(1u));
    BOOST_REQUIRE(instance.at(1).is_terminal());
}

BOOST_AUTO_TEST_CASE(headmap__setup__lifecycle__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x1122_u16));
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_SUITE_END()
