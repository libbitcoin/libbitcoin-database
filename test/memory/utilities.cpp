/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(memory_utilities_tests)

// It is presumed impossible for the page size to be zero, in which case this
// test should never fail on any platform, though an API failure returns zero.
BOOST_AUTO_TEST_CASE(memory_utilities__page_size__always__non_zero)
{
    BOOST_REQUIRE(is_nonzero(page_size()));
}

// It is not possible for the actual memory to be zero and an overflow will
// return max_uint64, so this test should never fail on any platform, though
// an API failure returns zero.
BOOST_AUTO_TEST_CASE(memory_utilities__system_memory__always__nonzero)
{
    BOOST_REQUIRE(is_nonzero(system_memory()));
}

BOOST_AUTO_TEST_SUITE_END()
