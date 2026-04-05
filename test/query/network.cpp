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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_network_tests, test::directory_setup_fixture)

const auto events_handler = [](auto, auto) {};

class query_access
  : public test::query_accessor
{
public:
    using base = test::query_accessor;
    using base::base;
    using base::get_fork;
    using base::get_blocks;
    using base::get_headers;
    using base::get_ancestry;
    using base::get_locator_span;
    using base::get_locator_start;
};

// get_headers

BOOST_AUTO_TEST_CASE(query_network__get_headers__empty_locator__returns_confirmed_headers)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto headers = query.get_headers(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(headers.size(), 3u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block1_hash);
    BOOST_REQUIRE_EQUAL(headers[1]->hash(), test::block2_hash);
    BOOST_REQUIRE_EQUAL(headers[2]->hash(), test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__genesis_locator__returns_all_confirmed)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    const hashes locator{ test::block0_hash };
    const auto headers = query.get_headers(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(headers.size(), 2u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block1_hash);
    BOOST_REQUIRE_EQUAL(headers[1]->hash(), test::block2_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__mid_chain_locator__starts_after_fork)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{ test::block1_hash, test::block0_hash };
    const auto headers = query.get_headers(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(headers.size(), 2u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block2_hash);
    BOOST_REQUIRE_EQUAL(headers[1]->hash(), test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__highest_first_locator__correct)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{ test::block2_hash, test::block1_hash, test::block0_hash };
    const auto headers = query.get_headers(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(headers.size(), 1u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__stop_hash__excludes_stop_and_later)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto headers = query.get_headers(locator, test::block2_hash, 10);
    BOOST_REQUIRE_EQUAL(headers.size(), 1u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block1_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__limit__respects_limit)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto headers = query.get_headers(locator, system::null_hash, 2);
    BOOST_REQUIRE_EQUAL(headers.size(), 2u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block1_hash);
    BOOST_REQUIRE_EQUAL(headers[1]->hash(), test::block2_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__no_confirmed_blocks__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const hashes locator{};
    const auto headers = query.get_headers(locator, system::null_hash, 10);
    BOOST_REQUIRE(headers.empty());
}

BOOST_AUTO_TEST_CASE(query_network__get_headers__reorg_terminal__returns_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));

    // Simulate a reorg by not confirming height 2, to_confirmed(2) is terminal.
    BOOST_REQUIRE(query.push_confirmed(1, false));

    const hashes locator{};
    const auto headers = query.get_headers(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(headers.size(), 1u);
    BOOST_REQUIRE_EQUAL(headers[0]->hash(), test::block1_hash);
}

// get_blocks

BOOST_AUTO_TEST_CASE(query_network__get_blocks__empty_locator__confirmed_headers)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto blocks = query.get_blocks(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(blocks.size(), 3u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block1_hash);
    BOOST_REQUIRE_EQUAL(blocks[1], test::block2_hash);
    BOOST_REQUIRE_EQUAL(blocks[2], test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__genesis_locator__all_confirmed)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    const hashes locator{ test::block0_hash };
    const auto blocks = query.get_blocks(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(blocks.size(), 2u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block1_hash);
    BOOST_REQUIRE_EQUAL(blocks[1], test::block2_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__mid_chain_locator__starts_after_fork)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{ test::block1_hash, test::block0_hash };
    const auto blocks = query.get_blocks(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(blocks.size(), 2u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block2_hash);
    BOOST_REQUIRE_EQUAL(blocks[1], test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__highest_first_locator__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{ test::block2_hash, test::block1_hash, test::block0_hash };
    const auto blocks = query.get_blocks(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(blocks.size(), 1u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__stop_hash__excludes_stop_and_after)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto blocks = query.get_blocks(locator, test::block2_hash, 10);
    BOOST_REQUIRE_EQUAL(blocks.size(), 1u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block1_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__limit__respects_limit)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto blocks = query.get_blocks(locator, system::null_hash, 2);
    BOOST_REQUIRE_EQUAL(blocks.size(), 2u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block1_hash);
    BOOST_REQUIRE_EQUAL(blocks[1], test::block2_hash);
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__no_confirmed_blocks__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const hashes locator{};
    const auto blocks = query.get_blocks(locator, system::null_hash, 10);
    BOOST_REQUIRE(blocks.empty());
}

BOOST_AUTO_TEST_CASE(query_network__get_blocks__reorg_terminal__returns_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));

    // Simulate a reorg by not confirming height 2, to_confirmed(2) is terminal.
    BOOST_REQUIRE(query.push_confirmed(1, false));

    const hashes locator{};
    const auto blocks = query.get_blocks(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(blocks.size(), 1u);
    BOOST_REQUIRE_EQUAL(blocks[0], test::block1_hash);
}

// get_locator_span

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__empty_locator__starts_after_genesis)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto span = query.get_locator_span(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(span.begin, 1u);
    BOOST_REQUIRE_EQUAL(span.end, 4u);
    BOOST_REQUIRE_EQUAL(span.size(), 3u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__genesis_locator__starts_after_genesis)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    const hashes locator{ test::block0_hash };
    const auto span = query.get_locator_span(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(span.begin, 1u);
    BOOST_REQUIRE_EQUAL(span.end, 3u);
    BOOST_REQUIRE_EQUAL(span.size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__mid_chain_locator__starts_after_fork)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{ test::block1_hash, test::block0_hash };
    const auto span = query.get_locator_span(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(span.begin, 2u);
    BOOST_REQUIRE_EQUAL(span.end, 4u);
    BOOST_REQUIRE_EQUAL(span.size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__stop_hash__limits_to_stop_exclusive)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto span = query.get_locator_span(locator, test::block2_hash, 10);
    BOOST_REQUIRE_EQUAL(span.begin, 1u);
    BOOST_REQUIRE_EQUAL(span.end, 2u);
    BOOST_REQUIRE_EQUAL(span.size(), 1u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__limit_smaller_than_range__respects_limit)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    const hashes locator{};
    const auto span = query.get_locator_span(locator, system::null_hash, 2);
    BOOST_REQUIRE_EQUAL(span.begin, 1u);
    BOOST_REQUIRE_EQUAL(span.end, 3u);
    BOOST_REQUIRE_EQUAL(span.size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__no_confirmed_blocks__empty_span)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));

    const hashes locator{};
    const auto span = query.get_locator_span(locator, system::null_hash, 10);
    BOOST_REQUIRE_EQUAL(span.begin, 1u);
    BOOST_REQUIRE_EQUAL(span.end, 1u);
    BOOST_REQUIRE_EQUAL(span.size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__stop_before_start__empty_span)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));

    const hashes locator{ test::block1_hash };
    const auto span = query.get_locator_span(locator, test::block0_hash, 10);
    BOOST_REQUIRE_EQUAL(span.begin, 2u);
    BOOST_REQUIRE_EQUAL(span.end, 2u);
    BOOST_REQUIRE_EQUAL(span.size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_span__large_limit__capped_by_top_confirmed)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));

    const hashes locator{};
    const auto span = query.get_locator_span(locator, system::null_hash, 1000);
    BOOST_REQUIRE_EQUAL(span.begin, 1u);
    BOOST_REQUIRE_EQUAL(span.end, 2u);
    BOOST_REQUIRE_EQUAL(span.size(), 1u);
}

// get_locator_start

BOOST_AUTO_TEST_CASE(query_network__get_locator_start__empty__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_locator_start({}), 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_start__genesis__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_locator_start({ test::block0_hash }), 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_start__unconfirmed__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE_EQUAL(query.get_locator_start({ test::block1_hash }), 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_locator_start__confirmed__first_match)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));
    BOOST_REQUIRE_EQUAL(query.get_locator_start({ system::null_hash, test::block2_hash, test::block1_hash }), 2u);
}

// get_ancestry

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__genesis__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    header_links ancestry{};
    BOOST_REQUIRE(query.get_ancestry(ancestry, 0, 0));
    BOOST_REQUIRE(ancestry.empty());
}

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__genesis__itself)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    header_links ancestry{};
    BOOST_REQUIRE(query.get_ancestry(ancestry, 0, 1));
    BOOST_REQUIRE_EQUAL(ancestry.size(), 1u);
    BOOST_REQUIRE_EQUAL(ancestry[0], 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__single_block__itself)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));

    header_links ancestry{};
    BOOST_REQUIRE(query.get_ancestry(ancestry, 1, 10));
    BOOST_REQUIRE_EQUAL(ancestry.size(), 2u);
    BOOST_REQUIRE_EQUAL(ancestry[0], 1u);
    BOOST_REQUIRE_EQUAL(ancestry[1], 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__linear_chain__full_ancestry)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    header_links ancestry{};
    BOOST_REQUIRE(query.get_ancestry(ancestry, 3, 10));
    BOOST_REQUIRE_EQUAL(ancestry.size(), 4u);
    BOOST_REQUIRE_EQUAL(ancestry[0], 3u);
    BOOST_REQUIRE_EQUAL(ancestry[1], 2u);
    BOOST_REQUIRE_EQUAL(ancestry[2], 1u);
    BOOST_REQUIRE_EQUAL(ancestry[3], 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__count_limit__truncated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    header_links ancestry{};
    BOOST_REQUIRE(query.get_ancestry(ancestry, 3, 2));
    BOOST_REQUIRE_EQUAL(ancestry.size(), 2u);
    BOOST_REQUIRE_EQUAL(ancestry[0], 3u);
    BOOST_REQUIRE_EQUAL(ancestry[1], 2u);
}

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__count_exceeds_height__full_to_genesis)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));

    header_links ancestry{};
    BOOST_REQUIRE(query.get_ancestry(ancestry, 2, 100));
    BOOST_REQUIRE_EQUAL(ancestry.size(), 3u);
    BOOST_REQUIRE_EQUAL(ancestry[0], 2u);
    BOOST_REQUIRE_EQUAL(ancestry[1], 1u);
    BOOST_REQUIRE_EQUAL(ancestry[2], 0u);
}

BOOST_AUTO_TEST_CASE(query_network__get_ancestry__nonexistent_link__false_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    query_access query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    header_links ancestry{};
    BOOST_REQUIRE(!query.get_ancestry(ancestry, 999, 10));
    BOOST_REQUIRE(ancestry.empty());
}

BOOST_AUTO_TEST_SUITE_END()
