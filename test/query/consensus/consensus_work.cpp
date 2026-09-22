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

// get_work
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__get_work__empty_states__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    uint256_t work{};
    const header_states states{};
    BOOST_REQUIRE(query.get_work(work, states));
    BOOST_REQUIRE_EQUAL(work, 0u);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_work__genesis__proof_of_bits)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    uint32_t bits{};
    BOOST_REQUIRE(query.get_bits(bits, 0));
    const header_states states{ { 0, code{} } };
    uint256_t work{};
    BOOST_REQUIRE(query.get_work(work, states));
    BOOST_REQUIRE_EQUAL(work, system::chain::header::proof(bits));
}

BOOST_AUTO_TEST_CASE(query_consensus__get_work__terminal_link__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    uint256_t work{};
    const header_states states{ { header_link::terminal, code{} } };
    BOOST_REQUIRE(!query.get_work(work, states));
}

// get_branch
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__get_branch__candidate_hash__empty_branch)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    header_states branch{};
    BOOST_REQUIRE(query.get_branch(branch, test::genesis.hash()));
    BOOST_REQUIRE(branch.empty());
}

BOOST_AUTO_TEST_CASE(query_consensus__get_branch__unknown_hash__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    header_states branch{};
    BOOST_REQUIRE(!query.get_branch(branch, test::block1.hash()));
}

BOOST_AUTO_TEST_CASE(query_consensus__get_branch__uncandidate_header__branch_of_one)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    header_states branch{};
    BOOST_REQUIRE(query.get_branch(branch, test::block1.hash()));
    BOOST_REQUIRE_EQUAL(branch.size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
