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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

// find_strong

BOOST_AUTO_TEST_CASE(query_consensus__find_strong__unconfirmed_duplicate__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));

    BOOST_REQUIRE(query.set(test::tx4));
    const auto unconfirmed = query.to_tx(test::tx4.hash(false));

    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));

    BOOST_REQUIRE_EQUAL(query.find_strong(unconfirmed),
        query.to_header(test::block_spend_1a.hash()));
}

// find_strong_spender_height

BOOST_AUTO_TEST_CASE(query_consensus__find_strong_spender_height__unspent__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));

    const system::chain::point point
    {
        test::block1a.transactions_ptr()->front()->hash(false), 0
    };

    BOOST_REQUIRE(query.find_strong_spender_height(point).is_terminal());
}

BOOST_AUTO_TEST_CASE(query_consensus__find_strong_spender_height__unconfirmed__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::tx5));

    const system::chain::point point
    {
        test::block1a.transactions_ptr()->front()->hash(false), 0
    };

    BOOST_REQUIRE(query.find_strong_spender_height(point).is_terminal());
}

BOOST_AUTO_TEST_CASE(query_consensus__find_strong_spender_height__confirmed__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));

    const system::chain::point point
    {
        test::block1a.transactions_ptr()->front()->hash(false), 0
    };

    const auto height = query.find_strong_spender_height(point);
    BOOST_REQUIRE(!height.is_terminal());
    BOOST_REQUIRE_EQUAL(height.value, 2u);
}

// is_strong

// The coinbase tx is strong.
BOOST_AUTO_TEST_CASE(query_consensus__is_strong__strong__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_strong_tx(0));
    BOOST_REQUIRE(query.is_strong_block(0));
}

BOOST_AUTO_TEST_CASE(query_consensus__is_strong__weak__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, {}, false, false));
    BOOST_REQUIRE(!query.is_strong_tx(1));
    BOOST_REQUIRE(!query.is_strong_block(1));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_strong_tx(1));
    BOOST_REQUIRE(query.is_strong_block(1));
}

// set_strong

BOOST_AUTO_TEST_CASE(query_consensus__set_strong__unassociated__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), context{}, {}, false));
    BOOST_REQUIRE(!query.set_strong(1));
    BOOST_REQUIRE(!query.set_unstrong(1));
}

BOOST_AUTO_TEST_CASE(query_consensus__set_strong__set_unstrong__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(query.is_confirmed_tx(1));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(query.is_confirmed_tx(2));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.set_unstrong(1));
    BOOST_REQUIRE(query.set_unstrong(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));
}

BOOST_AUTO_TEST_SUITE_END()
