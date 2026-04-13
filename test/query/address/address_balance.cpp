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

// Unconfirmed balance is always zero in the current implementation.

BOOST_FIXTURE_TEST_SUITE(query_address_tests, test::directory_setup_fixture)

// get_unconfirmed_balance
// get_confirmed_balance
// get_balance

BOOST_AUTO_TEST_CASE(query_address__get_balance__turbo_block1a_address0_confirmed_blocks__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));

    // block1a_address0 has 9 instances `script{ { { opcode::pick } } }`.
    // block1a_address0 has 4 confirmed instances (blocks 1a/2a/3a).
    // block1a (value: 0x18) is confirmed spent by block2a0.
    // block2a (value: 0x81) is confirmed unspent.
    // block2a (value: 0x81) is confirmed unspent.
    // block3a (value: 0x83) is confirmed unspent.
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    uint64_t unconfirmed{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_balance(cancel, unconfirmed, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(unconfirmed, 0u);

    uint64_t confirmed{};
    BOOST_REQUIRE(!query.get_confirmed_balance(cancel, confirmed, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(confirmed, 389u);

    confirmed = unconfirmed = 42;
    BOOST_REQUIRE(!query.get_balance(cancel, confirmed, unconfirmed, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(confirmed, 389u);
    BOOST_REQUIRE_EQUAL(unconfirmed, 0u);
}

BOOST_AUTO_TEST_CASE(query_address__get_balance__turbo_block1a_address0_unconfirmed_blocks__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));

    // block1a_address0 has 6 instances `script{ { { opcode::pick } } }`.
    BOOST_REQUIRE(test::setup_three_block_unconfirmed_address_store(query));

    uint64_t unconfirmed{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_balance(cancel, unconfirmed, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(unconfirmed, 0u);

    uint64_t confirmed{};
    BOOST_REQUIRE(!query.get_confirmed_balance(cancel, confirmed, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(confirmed, 0u);

    confirmed = unconfirmed = 42;
    BOOST_REQUIRE(!query.get_balance(cancel, confirmed, unconfirmed, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(confirmed, 0u);
    BOOST_REQUIRE_EQUAL(unconfirmed, 0u);
}

BOOST_AUTO_TEST_CASE(query_address__get_balance__block1a_address1_confirmed_blocks__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));

    // block1a_address1 is unique to that output instance `script{ { { opcode::roll } } }`.
    // block1a_address1 is spent by block2a1, tx4-1, and (confirmed double spent by) block3a1.
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    uint64_t unconfirmed{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_balance(cancel, unconfirmed, test::block1a_address1));
    BOOST_REQUIRE_EQUAL(unconfirmed, 0u);

    uint64_t confirmed{};
    BOOST_REQUIRE(!query.get_confirmed_balance(cancel, confirmed, test::block1a_address1));
    BOOST_REQUIRE_EQUAL(confirmed, 0u);

    confirmed = unconfirmed = 42;
    BOOST_REQUIRE(!query.get_balance(cancel, confirmed, unconfirmed, test::block1a_address1));
    BOOST_REQUIRE_EQUAL(confirmed, 0u);
    BOOST_REQUIRE_EQUAL(unconfirmed, 0u);
}

BOOST_AUTO_TEST_SUITE_END()
