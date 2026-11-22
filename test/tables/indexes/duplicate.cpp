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

BC_AUTO_TEST_SUITE(duplicate_tests)

BC_AUTO_TEST_CASE(duplicate__get_key__point__expected)
{
    const system::chain::point point{ system::one_hash, 0x00123456 };
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::duplicate instance{ head_store, body_store, 20 };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.put(point, table::duplicate::record{}));
    BC_REQUIRE(instance.get_key(0) == point);
}

BC_AUTO_TEST_CASE(duplicate__get_key__null_point__null_index)
{
    const system::chain::point point{ system::one_hash, system::chain::point::null_index };
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::duplicate instance{ head_store, body_store, 20 };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.put(point, table::duplicate::record{}));
    BC_REQUIRE(instance.get_key(0) == point);
}

BC_AUTO_TEST_SUITE_END()
