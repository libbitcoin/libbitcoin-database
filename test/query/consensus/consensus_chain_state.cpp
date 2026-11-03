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

// bip9 signaling
// ----------------------------------------------------------------------------
// Two block periods with a threshold of one, signaled at height 2: started at
// boundary 2, locked in at boundary 4 and active at boundary 6.

static system::settings signal_settings()
{
    system::settings settings{ system::chain::selection::mainnet };
    settings.forks.bip90 = false;
    settings.bip9_activation_period = 2;
    settings.bip9_bit0_deployment.start = 1;
    settings.bip9_bit0_deployment.timeout = max_uint32;
    settings.bip9_bit0_deployment.threshold = 1;
    return settings;
}

static system::chain::header signal_header(const system::hash_digest& previous, uint32_t version, uint32_t offset)
{
    return { version, previous, system::hash_digest{}, add1(test::genesis.header().timestamp()) + offset, test::genesis.header().bits(), 0 };
}

BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__bip9_signal__active_at_third_boundary)
{
    const auto settings = signal_settings();
    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto header1 = signal_header(test::genesis.hash(), 0x20000000, 1);
    const auto header2 = signal_header(header1.hash(), 0x20000001, 2);
    const auto header3 = signal_header(header2.hash(), 0x20000000, 3);
    const auto header4 = signal_header(header3.hash(), 0x20000000, 4);
    const auto header5 = signal_header(header4.hash(), 0x20000000, 5);
    const auto header6 = signal_header(header5.hash(), 0x20000000, 6);
    BC_REQUIRE(query.set(header1, context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(header2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(header3, context{ 0, 3, 0 }, {}, false));
    BC_REQUIRE(query.set(header4, context{ 0, 4, 0 }, {}, false));
    BC_REQUIRE(query.set(header5, context{ 0, 5, 0 }, {}, false));
    BC_REQUIRE(query.set(header6, context{ 0, 6, 0 }, {}, false));
    BC_REQUIRE(query.push_candidate(query.to_header(header1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header3.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header4.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header5.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header6.hash())));

    const auto state5 = query.get_candidate_chain_state(settings, 5);
    const auto state6 = query.get_candidate_chain_state(settings, 6);
    BC_REQUIRE(state5);
    BC_REQUIRE(state6);
    BC_REQUIRE(!to_bool(state5->flags() & system::chain::flags::bip68_rule));
    BC_REQUIRE(to_bool(state6->flags() & system::chain::flags::bip68_rule));
    BC_REQUIRE(to_bool(state6->flags() & system::chain::flags::bip112_rule));
    BC_REQUIRE(to_bool(state6->flags() & system::chain::flags::bip113_rule));
    BC_REQUIRE(!to_bool(state6->flags() & system::chain::flags::bip141_rule));
}

// The signal is in the branch, so the branch links are read.
BC_AUTO_TEST_CASE(query_consensus__get_confirmed_chain_state__bip9_signal_branch__active_at_third_boundary)
{
    const auto settings = signal_settings();
    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto header1 = signal_header(test::genesis.hash(), 0x20000000, 1);
    const auto branch2 = signal_header(header1.hash(), 0x20000001, 2);
    const auto branch3 = signal_header(branch2.hash(), 0x20000000, 3);
    const auto branch4 = signal_header(branch3.hash(), 0x20000000, 4);
    const auto branch5 = signal_header(branch4.hash(), 0x20000000, 5);
    const auto branch6 = signal_header(branch5.hash(), 0x20000000, 6);
    const auto header2 = signal_header(header1.hash(), 0x20000000, 7);
    BC_REQUIRE(query.set(header1, context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(header2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(branch2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(branch3, context{ 0, 3, 0 }, {}, false));
    BC_REQUIRE(query.set(branch4, context{ 0, 4, 0 }, {}, false));
    BC_REQUIRE(query.set(branch5, context{ 0, 5, 0 }, {}, false));
    BC_REQUIRE(query.set(branch6, context{ 0, 6, 0 }, {}, false));
    BC_REQUIRE(query.push_candidate(query.to_header(header1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header2.hash())));

    const auto state5 = query.get_confirmed_chain_state(settings, branch5.hash());
    const auto state6 = query.get_confirmed_chain_state(settings, branch6.hash());
    BC_REQUIRE(state5);
    BC_REQUIRE(state6);
    BC_REQUIRE_EQUAL(state6->height(), 6u);
    BC_REQUIRE(!to_bool(state5->flags() & system::chain::flags::bip68_rule));
    BC_REQUIRE(to_bool(state6->flags() & system::chain::flags::bip68_rule));
}

// Header promotion from a stored state matches the stored state at each height.
BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__bip9_signal_promoted__matches_stored)
{
    const auto settings = signal_settings();
    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto header1 = signal_header(test::genesis.hash(), 0x20000000, 1);
    const auto header2 = signal_header(header1.hash(), 0x20000000, 2);
    const auto header3 = signal_header(header2.hash(), 0x20000001, 3);
    const auto header4 = signal_header(header3.hash(), 0x20000000, 4);
    const auto header5 = signal_header(header4.hash(), 0x20000000, 5);
    const auto header6 = signal_header(header5.hash(), 0x20000000, 6);
    BC_REQUIRE(query.set(header1, context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(header2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(header3, context{ 0, 3, 0 }, {}, false));
    BC_REQUIRE(query.set(header4, context{ 0, 4, 0 }, {}, false));
    BC_REQUIRE(query.set(header5, context{ 0, 5, 0 }, {}, false));
    BC_REQUIRE(query.set(header6, context{ 0, 6, 0 }, {}, false));
    BC_REQUIRE(query.push_candidate(query.to_header(header1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header3.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header4.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header5.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header6.hash())));

    const auto stored3 = query.get_candidate_chain_state(settings, 3);
    const auto stored4 = query.get_candidate_chain_state(settings, 4);
    const auto stored5 = query.get_candidate_chain_state(settings, 5);
    const auto stored6 = query.get_candidate_chain_state(settings, 6);
    BC_REQUIRE(stored3);
    const system::chain::chain_state promoted4{ *stored3, header4, settings };
    const system::chain::chain_state promoted5{ promoted4, header5, settings };
    const system::chain::chain_state promoted6{ promoted5, header6, settings };
    BC_REQUIRE_EQUAL(promoted4.flags(), stored4->flags());
    BC_REQUIRE_EQUAL(promoted5.flags(), stored5->flags());
    BC_REQUIRE_EQUAL(promoted6.flags(), stored6->flags());
    BC_REQUIRE(!to_bool(promoted5.flags() & system::chain::flags::bip68_rule));
    BC_REQUIRE(to_bool(promoted6.flags() & system::chain::flags::bip68_rule));
}

// A start above every median time past leaves every period defined.
BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__bip9_signal_before_start__not_active)
{
    auto settings = signal_settings();
    settings.bip9_bit0_deployment.start = max_uint32 - 1u;
    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto header1 = signal_header(test::genesis.hash(), 0x20000001, 1);
    const auto header2 = signal_header(header1.hash(), 0x20000001, 2);
    const auto header3 = signal_header(header2.hash(), 0x20000001, 3);
    const auto header4 = signal_header(header3.hash(), 0x20000001, 4);
    const auto header5 = signal_header(header4.hash(), 0x20000001, 5);
    const auto header6 = signal_header(header5.hash(), 0x20000001, 6);
    BC_REQUIRE(query.set(header1, context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(header2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(header3, context{ 0, 3, 0 }, {}, false));
    BC_REQUIRE(query.set(header4, context{ 0, 4, 0 }, {}, false));
    BC_REQUIRE(query.set(header5, context{ 0, 5, 0 }, {}, false));
    BC_REQUIRE(query.set(header6, context{ 0, 6, 0 }, {}, false));
    BC_REQUIRE(query.push_candidate(query.to_header(header1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header3.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header4.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header5.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header6.hash())));

    const auto state = query.get_candidate_chain_state(settings, 6);
    BC_REQUIRE(state);
    BC_REQUIRE(!to_bool(state->flags() & system::chain::flags::bip68_rule));
}

// A bip9 deployment that times out as it starts fails despite signaling.
BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__bip9_signal_timed_out__not_active)
{
    auto settings = signal_settings();
    settings.bip9_bit0_deployment.timeout = settings.bip9_bit0_deployment.start;
    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto header1 = signal_header(test::genesis.hash(), 0x20000001, 1);
    const auto header2 = signal_header(header1.hash(), 0x20000001, 2);
    const auto header3 = signal_header(header2.hash(), 0x20000001, 3);
    const auto header4 = signal_header(header3.hash(), 0x20000001, 4);
    const auto header5 = signal_header(header4.hash(), 0x20000001, 5);
    const auto header6 = signal_header(header5.hash(), 0x20000001, 6);
    BC_REQUIRE(query.set(header1, context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(header2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(header3, context{ 0, 3, 0 }, {}, false));
    BC_REQUIRE(query.set(header4, context{ 0, 4, 0 }, {}, false));
    BC_REQUIRE(query.set(header5, context{ 0, 5, 0 }, {}, false));
    BC_REQUIRE(query.set(header6, context{ 0, 6, 0 }, {}, false));
    BC_REQUIRE(query.push_candidate(query.to_header(header1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header3.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header4.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header5.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header6.hash())));

    const auto state = query.get_candidate_chain_state(settings, 6);
    BC_REQUIRE(state);
    BC_REQUIRE(!to_bool(state->flags() & system::chain::flags::bip68_rule));
}

BC_AUTO_TEST_CASE(query_consensus__get_candidate_chain_state__bip9_no_signal__not_active)
{
    const auto settings = signal_settings();
    database::settings database_settings{ test::database_settings() };
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto header1 = signal_header(test::genesis.hash(), 0x20000000, 1);
    const auto header2 = signal_header(header1.hash(), 0x00000001, 2);
    const auto header3 = signal_header(header2.hash(), 0x20000000, 3);
    const auto header4 = signal_header(header3.hash(), 0x20000000, 4);
    const auto header5 = signal_header(header4.hash(), 0x20000000, 5);
    const auto header6 = signal_header(header5.hash(), 0x20000000, 6);
    BC_REQUIRE(query.set(header1, context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(header2, context{ 0, 2, 0 }, {}, false));
    BC_REQUIRE(query.set(header3, context{ 0, 3, 0 }, {}, false));
    BC_REQUIRE(query.set(header4, context{ 0, 4, 0 }, {}, false));
    BC_REQUIRE(query.set(header5, context{ 0, 5, 0 }, {}, false));
    BC_REQUIRE(query.set(header6, context{ 0, 6, 0 }, {}, false));
    BC_REQUIRE(query.push_candidate(query.to_header(header1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header3.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header4.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header5.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(header6.hash())));

    const auto state = query.get_candidate_chain_state(settings, 6);
    BC_REQUIRE(state);
    BC_REQUIRE(!to_bool(state->flags() & system::chain::flags::bip68_rule));
}

BC_AUTO_TEST_SUITE_END()
