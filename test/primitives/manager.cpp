/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include "../storage.hpp"

BOOST_AUTO_TEST_SUITE(manager_tests)

BOOST_AUTO_TEST_CASE(manager__size__empty_slab__zero)
{
    test::storage file;
    const manager<uint32_t, 1> instance(file);
    BOOST_REQUIRE(is_zero(instance.size()));
}

BOOST_AUTO_TEST_CASE(manager__size__empty_record__zero)
{
    test::storage file;
    const manager<uint32_t, 42> instance(file);
    BOOST_REQUIRE(is_zero(instance.size()));
}

BOOST_AUTO_TEST_CASE(manager__size__one_record__expected)
{
    data_chunk buffer(9, 0xff);
    test::storage file(buffer);
    const manager<uint32_t, 5> instance(file);
    BOOST_REQUIRE(instance.size());
}

BOOST_AUTO_TEST_SUITE_END()
