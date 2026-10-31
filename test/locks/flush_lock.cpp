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

BC_FIXTURE_TEST_SUITE(flush_lock_tests, test::directory_setup_fixture)

BC_AUTO_TEST_CASE(flush_lock__construct__file__expected)
{
    flush_lock instance(TEST_PATH);
    BC_REQUIRE_EQUAL(instance.file(), TEST_PATH);
}

BC_AUTO_TEST_CASE(flush_lock__try_lock__not_exists__true_created)
{
    BC_REQUIRE(!test::exists(TEST_PATH));

    flush_lock instance(TEST_PATH);
    BC_REQUIRE(instance.try_lock());
    BC_REQUIRE(test::exists(TEST_PATH));
}

BC_AUTO_TEST_CASE(flush_lock__try_lock__exists__false)
{
    BC_REQUIRE(test::create(TEST_PATH));

    flush_lock instance(TEST_PATH);
    BC_REQUIRE(!instance.try_lock());
}

BC_AUTO_TEST_CASE(flush_lock__try_unlock__not_exists__false)
{
    BC_REQUIRE(!test::exists(TEST_PATH));

    flush_lock instance(TEST_PATH);
    BC_REQUIRE(!instance.try_unlock());
}

BC_AUTO_TEST_CASE(flush_lock__try_unlock__exists__true_deleted)
{
    BC_REQUIRE(test::create(TEST_PATH));

    flush_lock instance(TEST_PATH);
    BC_REQUIRE(instance.try_unlock());
    BC_REQUIRE(!test::exists(TEST_PATH));
}

BC_AUTO_TEST_CASE(flush_lock__is_locked__not_exists__false)
{
    BC_REQUIRE(!test::exists(TEST_PATH));

    flush_lock instance(TEST_PATH);
    BC_REQUIRE(!instance.is_locked());
}

BC_AUTO_TEST_CASE(flush_lock__is_locked__exists__true)
{
    BC_REQUIRE(test::create(TEST_PATH));

    flush_lock instance(TEST_PATH);
    BC_REQUIRE(instance.is_locked());
}

BC_AUTO_TEST_SUITE_END()

