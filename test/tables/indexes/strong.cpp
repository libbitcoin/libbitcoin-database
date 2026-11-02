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
#include "../../test.hpp"
#include "../../mocks/chunk_storage.hpp"

BC_AUTO_TEST_SUITE(strong_tests)

using namespace system;
const table::strong::replace strong1{ {}, table::strong::merge(true, 0x0078f87f) };
const table::strong::replace strong2{ {}, table::strong::merge(false, 0x0078f87f) };
const auto expected_head = base16_chunk
(
    "00000000" // bucket[0] (fill)
    "7ff8f800" // bucket[1] 0x0078f87f | 0x00800000
    "7ff87800" // bucket[2] 0x0078f87f | 0x00000000
);

BC_AUTO_TEST_CASE(strong__set__two__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::strong instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(1u, strong1));
    BC_REQUIRE(instance.set(2u, strong2));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(head_file, expected_head);
    BC_REQUIRE(instance.close());
    BC_REQUIRE_EQUAL(head_file, expected_head);
}

BC_AUTO_TEST_CASE(strong__at__two__expected)
{
    auto head_file = expected_head;
    test::chunk_storage head_store{ head_file };
    table::strong instance{ head_store };
    BC_REQUIRE(instance.verify());
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE(!table::strong::positive(instance.at(0)));
    BC_REQUIRE_EQUAL(table::strong::header_fk(instance.at(0)), 0u);
    BC_REQUIRE(table::strong::positive(instance.at(1)));
    BC_REQUIRE_EQUAL(table::strong::header_fk(instance.at(1)), 0x0078f87fu);
    BC_REQUIRE(!table::strong::positive(instance.at(2)));
    BC_REQUIRE_EQUAL(table::strong::header_fk(instance.at(2)), 0x0078f87fu);
    BC_REQUIRE(!table::strong::positive(instance.at(3)));
    BC_REQUIRE_EQUAL(table::strong::header_fk(instance.at(3)), 0u);
}

BC_AUTO_TEST_CASE(strong__set__same_index__last_wins)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::strong instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(1u, strong1));
    BC_REQUIRE(table::strong::positive(instance.at(1)));
    BC_REQUIRE(instance.set(1u, strong2));
    BC_REQUIRE(!table::strong::positive(instance.at(1)));
    BC_REQUIRE_EQUAL(table::strong::header_fk(instance.at(1)), 0x0078f87fu);
    BC_REQUIRE_EQUAL(instance.count(), 2u);
}

BC_AUTO_TEST_SUITE_END()
