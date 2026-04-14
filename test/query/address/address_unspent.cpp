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
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    unspents out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_unspent(cancel, out, test::block1a_address0, true));

    // tx4/5 contain one address output each.
    // block1b (unconfirmed) contains 2 address outputs, both spent by block2b (0 unspent).
    // block2b (unconfirmed) contains 1 address output, so 3 total address outputs.
    BOOST_CHECK_EQUAL(out.size(), 3u);

    BOOST_CHECK_EQUAL(out.at(0).height, unspent::unused_height);
    BOOST_CHECK_EQUAL(out.at(0).position, unspent::unconfirmed_position);
    BOOST_CHECK_EQUAL(out.at(0).out.value(), 0x08u);
    BOOST_CHECK_EQUAL(out.at(0).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(0).out.point().hash(), test::tx4.hash(false));

    BOOST_CHECK_EQUAL(out.at(1).height, unspent::unused_height);
    BOOST_CHECK_EQUAL(out.at(1).position, unspent::unconfirmed_position);
    BOOST_CHECK_EQUAL(out.at(1).out.value(), 0x85u);
    BOOST_CHECK_EQUAL(out.at(1).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(1).out.point().hash(), test::tx5.hash(false));

    BOOST_CHECK_EQUAL(out.at(2).height, unspent::unused_height);
    BOOST_CHECK_EQUAL(out.at(2).position, unspent::unconfirmed_position);
    BOOST_CHECK_EQUAL(out.at(2).out.value(), 0xb2u);
    BOOST_CHECK_EQUAL(out.at(2).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(2).out.point().hash(), test::block2b.transactions_ptr()->at(0)->hash(false));

    // Arbitrary but consistent sort based on chain::point::operator<.
    BOOST_CHECK(out.at(0).out < out.at(1).out);
    BOOST_CHECK(out.at(1).out < out.at(2).out);

    out.clear();
    BOOST_REQUIRE(!query.get_confirmed_unspent(cancel, out, test::block1a_address0, true));

    // block1a is confirmed spent by block2a, block2a/3a are confirmed and contain 3 unspent.
    BOOST_CHECK_EQUAL(out.size(), 3u);

    BOOST_CHECK_EQUAL(out.at(0).height, 2u);
    BOOST_CHECK_EQUAL(out.at(0).position, 0u);
    BOOST_CHECK_EQUAL(out.at(0).out.value(), 0x81u);
    BOOST_CHECK_EQUAL(out.at(0).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(0).out.point().hash(), test::block2a.transactions_ptr()->at(0)->hash(false));

    BOOST_CHECK_EQUAL(out.at(1).height, 2u);
    BOOST_CHECK_EQUAL(out.at(1).position, 1u);
    BOOST_CHECK_EQUAL(out.at(1).out.value(), 0x81u);
    BOOST_CHECK_EQUAL(out.at(1).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(1).out.point().hash(), test::block2a.transactions_ptr()->at(1)->hash(false));

    BOOST_CHECK_EQUAL(out.at(2).height, 3u);
    BOOST_CHECK_EQUAL(out.at(2).position, 0u);
    BOOST_CHECK_EQUAL(out.at(2).out.value(), 0x83u);
    BOOST_CHECK_EQUAL(out.at(2).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(2).out.point().hash(), test::block3a.transactions_ptr()->at(0)->hash(false));

    out.clear();
    BOOST_REQUIRE(!query.get_unspent(cancel, out, test::block1a_address0, true));

    // As get_unconfirmed_unspent() and get_confirmed_unspent() are mutually-
    // exclusive, get_unspent() contains 3 + 3 = 6 address outputs.
    BOOST_CHECK_EQUAL(out.size(), 6u);

    // same as get_unconfirmed_unspent()

    BOOST_CHECK_EQUAL(out.at(0).height, 2u);
    BOOST_CHECK_EQUAL(out.at(0).position, 0u);
    BOOST_CHECK_EQUAL(out.at(0).out.value(), 0x81u);
    BOOST_CHECK_EQUAL(out.at(0).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(0).out.point().hash(), test::block2a.transactions_ptr()->at(0)->hash(false));

    BOOST_CHECK_EQUAL(out.at(1).height, 2u);
    BOOST_CHECK_EQUAL(out.at(1).position, 1u);
    BOOST_CHECK_EQUAL(out.at(1).out.value(), 0x81u);
    BOOST_CHECK_EQUAL(out.at(1).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(1).out.point().hash(), test::block2a.transactions_ptr()->at(1)->hash(false));

    BOOST_CHECK_EQUAL(out.at(2).height, 3u);
    BOOST_CHECK_EQUAL(out.at(2).position, 0u);
    BOOST_CHECK_EQUAL(out.at(2).out.value(), 0x83u);
    BOOST_CHECK_EQUAL(out.at(2).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(2).out.point().hash(), test::block3a.transactions_ptr()->at(0)->hash(false));

    // same as get_confirmed_unspent()

    BOOST_CHECK_EQUAL(out.at(3).height, unspent::unused_height);
    BOOST_CHECK_EQUAL(out.at(3).position, unspent::unconfirmed_position);
    BOOST_CHECK_EQUAL(out.at(3).out.value(), 0x08u);
    BOOST_CHECK_EQUAL(out.at(3).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(3).out.point().hash(), test::tx4.hash(false));

    BOOST_CHECK_EQUAL(out.at(4).height, unspent::unused_height);
    BOOST_CHECK_EQUAL(out.at(4).position, unspent::unconfirmed_position);
    BOOST_CHECK_EQUAL(out.at(4).out.value(), 0x85u);
    BOOST_CHECK_EQUAL(out.at(4).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(4).out.point().hash(), test::tx5.hash(false));

    BOOST_CHECK_EQUAL(out.at(5).height, unspent::unused_height);
    BOOST_CHECK_EQUAL(out.at(5).position, unspent::unconfirmed_position);
    BOOST_CHECK_EQUAL(out.at(5).out.value(), 0xb2u);
    BOOST_CHECK_EQUAL(out.at(5).out.point().index(), 0u);
    BOOST_CHECK_EQUAL(out.at(5).out.point().hash(), test::block2b.transactions_ptr()->at(0)->hash(false));
}

BOOST_AUTO_TEST_SUITE_END()
