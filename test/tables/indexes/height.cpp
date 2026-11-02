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

BC_AUTO_TEST_SUITE(height_tests)

using namespace system;
const table::height::link link1{ 0x00345678 };
const table::height::link link2{ 0x004def12 };
const auto expected_head = base16_chunk
(
    "78563400" // bucket[0]
    "12ef4d00" // bucket[1]
);

BC_AUTO_TEST_CASE(height__create__empty__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    table::height instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE(head_file.empty());
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE_EQUAL(instance.body_size(), zero);
    BC_REQUIRE(instance.at(0).is_terminal());
}

BC_AUTO_TEST_CASE(height__push__two__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    table::height instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(link1));
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(link2));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(head_file, expected_head);
    BC_REQUIRE(instance.close());
    BC_REQUIRE_EQUAL(head_file, expected_head);
}

BC_AUTO_TEST_CASE(height__at__two__expected)
{
    auto head_file = expected_head;
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    table::height instance{ head_store };
    BC_REQUIRE(instance.verify());
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(0), link1);
    BC_REQUIRE_EQUAL(instance.at(1), link2);
    BC_REQUIRE(instance.at(2).is_terminal());
}

BC_AUTO_TEST_CASE(height__truncate__push__rewrites_bucket)
{
    auto head_file = expected_head;
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    table::height instance{ head_store };
    BC_REQUIRE(!instance.truncate(3u));
    BC_REQUIRE(instance.truncate(1u));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE(instance.at(1).is_terminal());
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(link1));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(1), link1);
}

BC_AUTO_TEST_CASE(height__restore__expected__idempotent)
{
    auto head_file = expected_head;
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    table::height instance{ head_store };
    BC_REQUIRE(instance.backup());
    BC_REQUIRE(instance.restore());
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(head_file, expected_head);
}

BC_AUTO_TEST_SUITE_END()
