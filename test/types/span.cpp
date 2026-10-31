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

BC_AUTO_TEST_SUITE(span_tests)

using namespace system;

BC_AUTO_TEST_CASE(span__size__default__zero)
{
    const span instance{};
    BC_REQUIRE_EQUAL(instance.size(), 0u);
}

BC_AUTO_TEST_CASE(span__size__empty_range__zero)
{
    const span instance{ 5, 5 };
    BC_REQUIRE_EQUAL(instance.size(), 0u);
}

BC_AUTO_TEST_CASE(span__size__non_empty_range__expected)
{
    const span instance{ 10, 25 };
    BC_REQUIRE_EQUAL(instance.size(), 15u);
}

BC_AUTO_TEST_CASE(span__size__negative_range__zero)
{
    const span instance{ 100, 30 };
    BC_REQUIRE_EQUAL(instance.size(), 0u);
}

BC_AUTO_TEST_SUITE_END()
