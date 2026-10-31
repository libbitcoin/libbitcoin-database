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

BC_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__genesis__expected)
{
    const system::settings system_settings{ system::chain::selection::mainnet };
    const system::chain::context expected
    {
        16523u,
        test::genesis.header().timestamp(),
        0u,
        0u,
        0u,
        0u
    };

    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto state = query.get_candidate_chain_state(system_settings);
    BC_REQUIRE(state);
    BC_REQUIRE_EQUAL(state->flags(), expected.flags);
    BC_REQUIRE_EQUAL(state->height(), expected.height);
    BC_REQUIRE_EQUAL(state->timestamp(), expected.timestamp);
    BC_REQUIRE_EQUAL(state->work_required(), expected.work_required);
    BC_REQUIRE_EQUAL(state->median_time_past(), expected.median_time_past);
    BC_REQUIRE_EQUAL(state->minimum_block_version(), expected.minimum_block_version);
    BC_REQUIRE_EQUAL(state->cumulative_work(), test::genesis.header().proof());
    BC_REQUIRE(state->context() == expected);
}

BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__block1__expected)
{
    const system::settings system_settings{ system::chain::selection::mainnet };
    const system::chain::context expected
    {
        16523u,                             // flags
        test::block1.header().timestamp(),  // timestamp
        test::genesis.header().timestamp(), // mtp
        1u,                                 // height
        0u,                                 // minimum_block_version
        486604799u,                         // work_required
        test::genesis.header().timestamp()  // previous timestamp
    };

    // Not actually contributory.
    const database::context context
    {
        expected.flags,
        system::possible_narrow_cast<uint32_t>(expected.height),
        expected.median_time_past
    };

    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    const auto work = test::genesis.header().proof() + test::block1.header().proof();
    BC_REQUIRE(query.set(test::block1, context, work, true, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));

    const auto state = query.get_candidate_chain_state(system_settings);
    BC_REQUIRE(state);
    BC_REQUIRE_EQUAL(state->flags(), expected.flags);
    BC_REQUIRE_EQUAL(state->height(), expected.height);
    BC_REQUIRE_EQUAL(state->timestamp(), expected.timestamp);
    BC_REQUIRE_EQUAL(state->work_required(), expected.work_required);
    BC_REQUIRE_EQUAL(state->median_time_past(), expected.median_time_past);
    BC_REQUIRE_EQUAL(state->minimum_block_version(), expected.minimum_block_version);
    BC_REQUIRE_EQUAL(state->previous_timestamp(), expected.previous_timestamp);
    BC_REQUIRE_EQUAL(state->cumulative_work(), test::genesis.header().proof() + test::block1.header().proof());
    BC_REQUIRE(state->context() == expected);
}

BC_AUTO_TEST_SUITE_END()
