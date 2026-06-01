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

BOOST_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

static void set_validated_fork(test::query_t& query)
{
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));

    const auto link1 = query.to_header(test::block1.hash());
    const auto link2 = query.to_header(test::block2.hash());
    BOOST_REQUIRE(query.push_candidate(link1));
    BOOST_REQUIRE(query.push_candidate(link2));
    BOOST_REQUIRE(query.set_block_valid(link1));
    BOOST_REQUIRE(query.set_block_valid(link2));
}

static void set_optional_indexes(test::query_t& query,
    const header_link& link, const system::chain::block& block)
{
    BOOST_REQUIRE(query.set_filter_body(link, block));
    BOOST_REQUIRE(query.set_filter_head(link));
    BOOST_REQUIRE(query.set_silent(link, block));
}

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__optional_unindexed__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    set_validated_fork(query);

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE(fork.empty());
}

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__optional_indexed__returns_validated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    set_validated_fork(query);

    const auto link1 = query.to_header(test::block1.hash());
    const auto link2 = query.to_header(test::block2.hash());
    set_optional_indexes(query, link1, test::block1);
    set_optional_indexes(query, link2, test::block2);

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE_EQUAL(fork.size(), 2u);
    BOOST_REQUIRE(fork[0].link == link1);
    BOOST_REQUIRE(fork[1].link == link2);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__silent_below_start__not_required)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.silent_start_height = 2;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    set_validated_fork(query);

    const auto link1 = query.to_header(test::block1.hash());
    const auto link2 = query.to_header(test::block2.hash());
    BOOST_REQUIRE(query.set_filter_body(link1, test::block1));
    BOOST_REQUIRE(query.set_filter_head(link1));
    BOOST_REQUIRE(query.set_filter_body(link2, test::block2));
    BOOST_REQUIRE(query.set_filter_head(link2));

    size_t fork_point{};
    auto fork = query.get_validated_fork(fork_point);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE_EQUAL(fork.size(), 1u);
    BOOST_REQUIRE(fork.front().link == link1);

    BOOST_REQUIRE(query.set_silent(link2, test::block2));
    fork = query.get_validated_fork(fork_point);
    BOOST_REQUIRE_EQUAL(fork.size(), 2u);
    BOOST_REQUIRE(fork[0].link == link1);
    BOOST_REQUIRE(fork[1].link == link2);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_validated_fork__silent_disabled__filter_indexed__returns_validated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.silent_buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    set_validated_fork(query);

    const auto link1 = query.to_header(test::block1.hash());
    const auto link2 = query.to_header(test::block2.hash());
    BOOST_REQUIRE(query.set_filter_body(link1, test::block1));
    BOOST_REQUIRE(query.set_filter_head(link1));
    BOOST_REQUIRE(query.set_filter_body(link2, test::block2));
    BOOST_REQUIRE(query.set_filter_head(link2));

    size_t fork_point{};
    const auto fork = query.get_validated_fork(fork_point);
    BOOST_REQUIRE_EQUAL(fork_point, 0u);
    BOOST_REQUIRE_EQUAL(fork.size(), 2u);
    BOOST_REQUIRE(fork[0].link == link1);
    BOOST_REQUIRE(fork[1].link == link2);
}

BOOST_AUTO_TEST_SUITE_END()
