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

BC_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// tables enum

BC_AUTO_TEST_CASE(store__tables__each_table__named)
{
    const auto count = add1(static_cast<size_t>(table_t::filter_tx_body));
    BC_REQUIRE_EQUAL(store<database::mmap>::tables.size(), count);
}

BC_AUTO_TEST_SUITE_END()
