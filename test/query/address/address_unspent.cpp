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

// get_unconfirmed_unspent
// get_confirmed_unspent
// get_unspent

BOOST_AUTO_TEST_CASE(query_address__get_unspent__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    unspents out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_unspent(cancel, out, test::genesis_address0));
    BOOST_REQUIRE_EQUAL(out.size(), 0u);

    out.clear();
    BOOST_REQUIRE(!query.get_confirmed_unspent(cancel, out, test::genesis_address0));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.at(0).height, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).position, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).out.value(), 5'000'000'000u);
    BOOST_REQUIRE_EQUAL(out.at(0).out.point().index(), 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).out.point().hash(), test::genesis.transactions_ptr()->at(0)->hash(false));

    out.clear();
    BOOST_REQUIRE(!query.get_unspent(cancel, out, test::genesis_address0));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.at(0).height, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).position, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).out.value(), 5'000'000'000u);
    BOOST_REQUIRE_EQUAL(out.at(0).out.point().index(), 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).out.point().hash(), test::genesis.transactions_ptr()->at(0)->hash(false));
}

BOOST_AUTO_TEST_CASE(query_address__get_unspent__turbo_genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    unspents out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_unspent(cancel, out, test::genesis_address0, true));
    BOOST_CHECK_EQUAL(out.size(), 0u);

    out.clear();
    BOOST_REQUIRE(!query.get_confirmed_unspent(cancel, out, test::genesis_address0, true));
    BOOST_CHECK_EQUAL(out.size(), 1u);

    out.clear();
    BOOST_REQUIRE(!query.get_unspent(cancel, out, test::genesis_address0, true));
    BOOST_CHECK_EQUAL(out.size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
