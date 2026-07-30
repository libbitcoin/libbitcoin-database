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

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__bypassed_filters_pending__stops_at_first_missing)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.filter_enabled());
    BOOST_REQUIRE(query.set(test::block1,
        database::context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2,
        database::context{ 0, 2, 0 }, false, false));

    const auto link1 = query.to_header(test::block1_hash);
    const auto link2 = query.to_header(test::block2_hash);
    BOOST_REQUIRE(query.push_candidate(link1));
    BOOST_REQUIRE(query.push_candidate(link2));

    size_t fork_point{};
    auto fork = query.get_validated_fork(fork_point, 2);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE(fork.empty());

    BOOST_REQUIRE(query.set_filter_body(link1, test::block1));
    fork = query.get_validated_fork(fork_point, 2);
    BOOST_REQUIRE_EQUAL(fork.size(), 1u);
    BOOST_REQUIRE(fork.front().link == link1);
    BOOST_REQUIRE(fork.front().ec == error::bypassed);

    BOOST_REQUIRE(query.set_filter_body(link2, test::block2));
    fork = query.get_validated_fork(fork_point, 2);
    BOOST_REQUIRE_EQUAL(fork.size(), 2u);
    BOOST_REQUIRE(fork.front().link == link1);
    BOOST_REQUIRE(fork.back().link == link2);
    BOOST_REQUIRE(fork.front().ec == error::bypassed);
    BOOST_REQUIRE(fork.back().ec == error::bypassed);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__filters_disabled__bypass_unchanged)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.filter_tx.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.filter_enabled());
    BOOST_REQUIRE(query.set(test::block1,
        database::context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2,
        database::context{ 0, 2, 0 }, false, false));

    const auto link1 = query.to_header(test::block1_hash);
    const auto link2 = query.to_header(test::block2_hash);
    BOOST_REQUIRE(query.push_candidate(link1));
    BOOST_REQUIRE(query.push_candidate(link2));

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point, 2);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE_EQUAL(fork.size(), 2u);
    BOOST_REQUIRE(fork.front().link == link1);
    BOOST_REQUIRE(fork.back().link == link2);
    BOOST_REQUIRE(fork.front().ec == error::bypassed);
    BOOST_REQUIRE(fork.back().ec == error::bypassed);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__milestone_filter_pending__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.filter_enabled());
    BOOST_REQUIRE(query.set(test::block1,
        database::context{ 0, 1, 0 }, true, false));

    const auto link = query.to_header(test::block1_hash);
    BOOST_REQUIRE(query.push_candidate(link));

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE(fork.empty());
}

BOOST_AUTO_TEST_SUITE_END()
