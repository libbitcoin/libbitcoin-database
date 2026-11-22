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

BC_AUTO_TEST_SUITE(memory_utilities_tests)

// Presumed impossible for page size to be zero, so this test should never fail
// on any platform, though API failure returns zero.
BC_AUTO_TEST_CASE(memory_utilities__page_size__always__non_zero)
{
    BC_REQUIRE(is_nonzero(page_size()));
}

// Actual memory cannot be zero and overflow returns max_uint64, so this test
// should never fail on any platform, though API failure returns zero.
BC_AUTO_TEST_CASE(memory_utilities__system_memory__always__nonzero)
{
    BC_REQUIRE(is_nonzero(system_memory()));
}

BC_AUTO_TEST_CASE(memory_utilities__physical_memory__always__not_less_than_system_memory)
{
    BC_REQUIRE_GE(physical_memory(), system_memory());
}

BC_AUTO_TEST_SUITE_END()
