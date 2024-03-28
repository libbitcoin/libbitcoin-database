/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

struct query_context_setup_fixture
{
    DELETE_COPY_MOVE(query_context_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_context_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_context_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_context_tests, query_context_setup_fixture)

const auto events = [](auto, auto) {};

BOOST_AUTO_TEST_CASE(query_context__get_candidate_chain_state__genesis__expected)
{
    const system::settings system_settings{ system::chain::selection::mainnet };
    const uint256_t expected_cumulative_work{};
    const system::chain::context expected
    {
        131211u,
        test::genesis.header().timestamp(),
        0u,
        0u,
        1u,
        0u
    };

    database::settings database_settings{};
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto state = query.get_candidate_chain_state(system_settings);
    BOOST_REQUIRE(state);
    BOOST_REQUIRE_EQUAL(state->flags(), expected.flags);
    BOOST_REQUIRE_EQUAL(state->height(), expected.height);
    BOOST_REQUIRE_EQUAL(state->timestamp(), expected.timestamp);
    BOOST_REQUIRE_EQUAL(state->work_required(), expected.work_required);
    BOOST_REQUIRE_EQUAL(state->median_time_past(), expected.median_time_past);
    BOOST_REQUIRE_EQUAL(state->minimum_block_version(), expected.minimum_block_version);
    BOOST_REQUIRE_EQUAL(state->cumulative_work(), test::genesis.header().proof());
    BOOST_REQUIRE(state->context() == expected);
}

BOOST_AUTO_TEST_CASE(query_context__get_candidate_chain_state__block1__expected)
{
    const system::settings system_settings{ system::chain::selection::mainnet };
    const system::chain::context expected
    {
        131211u,                            // flags
        test::block1.header().timestamp(),  // timestamp
        test::genesis.header().timestamp(), // mtp
        1u,                                 // height
        1u,                                 // minimum_block_version
        486604799u                          // work_required
    };

    // Not actually contributory.
    const database::context context
    {
        expected.flags,
        system::possible_narrow_cast<uint32_t>(expected.height),
        expected.median_time_past
    };

    database::settings database_settings{};
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));

    const auto state = query.get_candidate_chain_state(system_settings);
    BOOST_REQUIRE(state);
    BOOST_REQUIRE_EQUAL(state->flags(), expected.flags);
    BOOST_REQUIRE_EQUAL(state->height(), expected.height);
    BOOST_REQUIRE_EQUAL(state->timestamp(), expected.timestamp);
    BOOST_REQUIRE_EQUAL(state->work_required(), expected.work_required);
    BOOST_REQUIRE_EQUAL(state->median_time_past(), expected.median_time_past);
    BOOST_REQUIRE_EQUAL(state->minimum_block_version(), expected.minimum_block_version);
    BOOST_REQUIRE_EQUAL(state->cumulative_work(), test::genesis.header().proof() + test::block1.header().proof());
    BOOST_REQUIRE(state->context() == expected);
}

BOOST_AUTO_TEST_CASE(query_context__get_confirmed_chain_state__testnet_genesis__expected)
{
    const auto genesis = system::settings{ system::chain::selection::testnet }.genesis_block;
    const system::settings system_settings{ system::chain::selection::testnet };
    const uint256_t expected_cumulative_work{};
    const system::chain::context expected
    {
        131210u,
        genesis.header().timestamp(),
        0u,
        0u,
        1u,
        0u
    };

    database::settings database_settings{};
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(genesis));

    const auto state = query.get_confirmed_chain_state(system_settings);
    BOOST_REQUIRE(state);
    BOOST_REQUIRE_EQUAL(state->flags(), expected.flags);
    BOOST_REQUIRE_EQUAL(state->height(), expected.height);
    BOOST_REQUIRE_EQUAL(state->timestamp(), expected.timestamp);
    BOOST_REQUIRE_EQUAL(state->work_required(), expected.work_required);
    BOOST_REQUIRE_EQUAL(state->median_time_past(), expected.median_time_past);
    BOOST_REQUIRE_EQUAL(state->minimum_block_version(), expected.minimum_block_version);
    BOOST_REQUIRE_EQUAL(state->cumulative_work(), genesis.header().proof());
    BOOST_REQUIRE(state->context() == expected);
}

BOOST_AUTO_TEST_CASE(query_context__get_confirmed_chain_state__block1__expected)
{
    const system::settings system_settings{ system::chain::selection::mainnet };
    const system::chain::context expected
    {
        131211u,                            // flags
        test::block1.header().timestamp(),  // timestamp
        test::genesis.header().timestamp(), // mtp
        1u,                                 // height
        1u,                                 // minimum_block_version
        486604799u                          // work_required
    };

    // Not actually contributory.
    const database::context context
    {
        expected.flags,
        system::possible_narrow_cast<uint32_t>(expected.height),
        expected.median_time_past
    };

    database::settings database_settings{};
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash())));

    const auto state = query.get_confirmed_chain_state(system_settings);
    BOOST_REQUIRE(state);
    BOOST_REQUIRE_EQUAL(state->flags(), expected.flags);
    BOOST_REQUIRE_EQUAL(state->height(), expected.height);
    BOOST_REQUIRE_EQUAL(state->timestamp(), expected.timestamp);
    BOOST_REQUIRE_EQUAL(state->work_required(), expected.work_required);
    BOOST_REQUIRE_EQUAL(state->median_time_past(), expected.median_time_past);
    BOOST_REQUIRE_EQUAL(state->minimum_block_version(), expected.minimum_block_version);
    BOOST_REQUIRE_EQUAL(state->cumulative_work(), test::genesis.header().proof() + test::block1.header().proof());
    BOOST_REQUIRE(state->context() == expected);
}

BOOST_AUTO_TEST_SUITE_END()
