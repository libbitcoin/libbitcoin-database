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

BC_AUTO_TEST_CASE(query_consensus__get_validated_fork__bypassed_filters_pending__stops_at_first_missing)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.filter_enabled());
    BC_REQUIRE(query.set(test::block1, database::context{ 0, 1, 0 }, {}, true, false));
    BC_REQUIRE(query.set(test::block2, database::context{ 0, 2, 0 }, {}, true, false));

    const auto link1 = query.to_header(test::block1_hash);
    const auto link2 = query.to_header(test::block2_hash);
    BC_REQUIRE(query.push_candidate(link1));
    BC_REQUIRE(query.push_candidate(link2));

    size_t fork_point{};
    auto fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork_point, 0u);
    BC_REQUIRE(fork.empty());

    BC_REQUIRE(query.set_filter_body(link1, test::block1));
    fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork.size(), 1u);
    BC_REQUIRE(fork.front().link == link1);
    BC_REQUIRE(is_bypass(fork.front().state));

    BC_REQUIRE(query.set_filter_body(link2, test::block2));
    fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork.size(), 2u);
    BC_REQUIRE(fork.front().link == link1);
    BC_REQUIRE(fork.back().link == link2);
    BC_REQUIRE(is_bypass(fork.front().state));
    BC_REQUIRE(is_bypass(fork.back().state));
}

BC_AUTO_TEST_CASE(query_consensus__get_validated_fork__filters_disabled__bypass_unchanged)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.filter_tx.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(!query.filter_enabled());
    BC_REQUIRE(query.set(test::block1, database::context{ 0, 1, 0 }, {}, true, false));
    BC_REQUIRE(query.set(test::block2, database::context{ 0, 2, 0 }, {}, true, false));

    const auto link1 = query.to_header(test::block1_hash);
    const auto link2 = query.to_header(test::block2_hash);
    BC_REQUIRE(query.push_candidate(link1));
    BC_REQUIRE(query.push_candidate(link2));

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork_point, 0u);
    BC_REQUIRE_EQUAL(fork.size(), 2u);
    BC_REQUIRE(fork.front().link == link1);
    BC_REQUIRE(fork.back().link == link2);
    BC_REQUIRE(is_bypass(fork.front().state));
    BC_REQUIRE(is_bypass(fork.back().state));
}

BC_AUTO_TEST_CASE(query_consensus__get_validated_fork__bypassed_unscanned__stops_at_unscanned)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.filter_tx.buckets = 0;
    settings.initialize(system::settings{ system::chain::selection::regtest }, false, 2);
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(!query.filter_enabled());
    BC_REQUIRE_EQUAL(query.silent_start_height(), 2u);
    BC_REQUIRE(query.set(test::block1, database::context{ system::chain::flags::bip341_rule, 1, 0 }, {}, true, false));
    BC_REQUIRE(query.set(test::block2, database::context{ system::chain::flags::bip341_rule, 2, 0 }, {}, true, false));

    const auto link1 = query.to_header(test::block1_hash);
    const auto link2 = query.to_header(test::block2_hash);
    BC_REQUIRE(query.push_candidate(link1));
    BC_REQUIRE(query.push_candidate(link2));

    size_t fork_point{};
    auto fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork_point, 0u);
    BC_REQUIRE_EQUAL(fork.size(), 1u);
    BC_REQUIRE(fork.front().link == link1);
    BC_REQUIRE(is_bypass(fork.front().state));

    BC_REQUIRE(query.set_block_scanned(link2));
    fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork.size(), 2u);
    BC_REQUIRE(fork.back().link == link2);
    BC_REQUIRE(is_bypass(fork.back().state));
}

BC_AUTO_TEST_CASE(query_consensus__get_validated_fork__milestone_filter_pending__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.filter_enabled());
    BC_REQUIRE(query.set(test::block1, database::context{ 0, 1, 0 }, {}, true, false));

    const auto link = query.to_header(test::block1_hash);
    BC_REQUIRE(query.push_candidate(link));

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point);
    BC_REQUIRE_EQUAL(fork_point, 0u);
    BC_REQUIRE(fork.empty());
}

// get_confirmed_height

BC_AUTO_TEST_CASE(query_consensus__get_confirmed_height__push_pop_confirmed__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BC_REQUIRE(!query.get_confirmed_height(0).is_terminal());
    BC_REQUIRE(query.get_confirmed_height(1).is_terminal());
    BC_REQUIRE(query.get_confirmed_height(2).is_terminal());

    BC_REQUIRE(query.push_confirmed(1, false));
    BC_REQUIRE(!query.get_confirmed_height(1).is_terminal());

    BC_REQUIRE(query.push_confirmed(2, false));
    BC_REQUIRE(!query.get_confirmed_height(2).is_terminal());

    BC_REQUIRE(query.pop_confirmed());
    BC_REQUIRE(!query.get_confirmed_height(0).is_terminal());
    BC_REQUIRE(!query.get_confirmed_height(1).is_terminal());
    BC_REQUIRE(query.get_confirmed_height(2).is_terminal());

    BC_REQUIRE(!!query.pop_confirmed());
    BC_REQUIRE(!query.get_confirmed_height(0).is_terminal());
    BC_REQUIRE(query.get_confirmed_height(1).is_terminal());
    BC_REQUIRE(query.get_confirmed_height(2).is_terminal());
}

// get_fork

BC_AUTO_TEST_CASE(query_consensus__get_fork__initialized__0)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    BC_REQUIRE_EQUAL(query.get_fork(), 0u);
}

BC_AUTO_TEST_CASE(query_consensus__get_fork__candidate_ahead__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BC_REQUIRE_EQUAL(query.get_fork(), 1u);
}

BC_AUTO_TEST_CASE(query_consensus__get_fork__confirmed_ahead__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BC_REQUIRE_EQUAL(query.get_fork(), 1u);
}

BC_AUTO_TEST_SUITE_END()
