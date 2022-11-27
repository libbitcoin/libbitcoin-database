/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(query_tests)

BOOST_AUTO_TEST_CASE(query__set_header__default__expected)
{
    settings settings_{};
    settings_.dir = TEST_DIRECTORY;
    store store_{ settings_ };
    query<store> instance{ store_ };

    context context_{};
    system::chain::header header_{};
    BOOST_REQUIRE(!instance.set_header(header_, context_));
}

BOOST_AUTO_TEST_CASE(query__set_tx__default__expected)
{
    settings settings_{};
    settings_.dir = TEST_DIRECTORY;
    store store_{ settings_ };
    query<store> instance{ store_ };

    system::chain::transaction transaction_{};
    BOOST_REQUIRE(!instance.set_tx(transaction_));
}

BOOST_AUTO_TEST_SUITE_END()
