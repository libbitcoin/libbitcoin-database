/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_address_tests, test::directory_setup_fixture)

// get_unconfirmed_balance
// get_confirmed_balance
// get_balance

BOOST_AUTO_TEST_CASE(query_address__get_balance__turbo_genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint64_t combined{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_balance(cancel, combined, test::genesis_address, true));
    BOOST_REQUIRE_EQUAL(combined, 5000000000u);

    uint64_t confirmed{};
    BOOST_REQUIRE(!query.get_confirmed_balance(cancel, confirmed, test::genesis_address, true));
    BOOST_REQUIRE_EQUAL(confirmed, 5000000000u);

    confirmed = combined = 42;
    BOOST_REQUIRE(!query.get_balance(cancel, confirmed, combined, test::genesis_address, true));
    BOOST_REQUIRE_EQUAL(confirmed, 5000000000u);
    BOOST_REQUIRE_EQUAL(combined, 5000000000u);
}

BOOST_AUTO_TEST_CASE(query_address__get_balance__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint64_t combined{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_balance(cancel, combined, test::genesis_address));
    BOOST_REQUIRE_EQUAL(combined, 5000000000u);

    uint64_t confirmed{};
    BOOST_REQUIRE(!query.get_confirmed_balance(cancel, confirmed, test::genesis_address));
    BOOST_REQUIRE_EQUAL(confirmed, 5000000000u);

    confirmed = combined = 42;
    BOOST_REQUIRE(!query.get_balance(cancel, confirmed, combined, test::genesis_address));
    BOOST_REQUIRE_EQUAL(confirmed, 5000000000u);
    BOOST_REQUIRE_EQUAL(combined, 5000000000u);
}

BOOST_AUTO_TEST_SUITE_END()
