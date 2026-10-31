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

BC_AUTO_TEST_SUITE(difference_set_tests)

using namespace system;

// Members of a set, in element order.
static std_vector<size_t> members(const difference_set& set) NOEXCEPT
{
    std_vector<size_t> out{};
    for (size_t index{}; index < set.words(); ++index)
        for (auto bits = set.at(index); !is_zero(bits);
            bits = bit_and(bits, sub1(bits)))
            out.push_back(index * difference_set::word_bits +
                right_zeros(bits));

    return out;
}

BC_AUTO_TEST_CASE(difference_set__size__default__zero)
{
    const difference_set instance{};
    BC_REQUIRE_EQUAL(instance.size(), zero);
    BC_REQUIRE_EQUAL(instance.words(), zero);
}

BC_AUTO_TEST_CASE(difference_set__words__partial_word__ceilinged)
{
    const difference_set instance{ 65 };
    BC_REQUIRE_EQUAL(instance.size(), 65u);
    BC_REQUIRE_EQUAL(instance.words(), 2u);
}

BC_AUTO_TEST_CASE(difference_set__members__unpresented__empty)
{
    const difference_set instance{ 256 };
    BC_REQUIRE(members(instance).empty());
}

BC_AUTO_TEST_CASE(difference_set__toggle__once__member)
{
    difference_set instance{ 256 };
    instance.toggle(42);
    BC_REQUIRE_EQUAL(members(instance), std_vector<size_t>{ 42 });
}

BC_AUTO_TEST_CASE(difference_set__toggle__twice__cancelled)
{
    difference_set instance{ 256 };
    instance.toggle(42);
    instance.toggle(42);
    BC_REQUIRE(members(instance).empty());
}

BC_AUTO_TEST_CASE(difference_set__toggle__thrice__member)
{
    difference_set instance{ 256 };
    instance.toggle(42);
    instance.toggle(42);
    instance.toggle(42);
    BC_REQUIRE_EQUAL(members(instance), std_vector<size_t>{ 42 });
}

BC_AUTO_TEST_CASE(difference_set__toggle__distinct__all_members)
{
    difference_set instance{ 256 };
    instance.toggle(0);
    instance.toggle(63);
    instance.toggle(64);
    instance.toggle(255);
    const std_vector<size_t> expected{ 0, 63, 64, 255 };
    BC_REQUIRE_EQUAL(members(instance), expected);
}

BC_AUTO_TEST_CASE(difference_set__toggle__paired__residue_only)
{
    difference_set instance{ 256 };
    instance.toggle(10);
    instance.toggle(20);
    instance.toggle(30);
    instance.toggle(20);
    instance.toggle(10);
    BC_REQUIRE_EQUAL(members(instance), std_vector<size_t>{ 30 });
}

BC_AUTO_TEST_CASE(difference_set__toggle__word_boundary__independent)
{
    difference_set instance{ 128 };
    instance.toggle(63);
    instance.toggle(64);
    instance.toggle(63);
    BC_REQUIRE_EQUAL(members(instance), std_vector<size_t>{ 64 });
}

BC_AUTO_TEST_CASE(difference_set__at__unset_word__zero)
{
    const difference_set instance{ 128 };
    BC_REQUIRE_EQUAL(instance.at(0), 0_u64);
    BC_REQUIRE_EQUAL(instance.at(1), 0_u64);
}

BC_AUTO_TEST_CASE(difference_set__at__set_elements__expected_word)
{
    difference_set instance{ 128 };
    instance.toggle(0);
    instance.toggle(2);
    BC_REQUIRE_EQUAL(instance.at(0), 0b101_u64);
    BC_REQUIRE_EQUAL(instance.at(1), 0_u64);
}

BC_AUTO_TEST_SUITE_END()
