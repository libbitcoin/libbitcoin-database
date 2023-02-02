/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

struct query_initialization_setup_fixture
{
    DELETE_COPY_MOVE(query_initialization_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_initialization_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_initialization_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_initialization_tests, query_initialization_setup_fixture)

BOOST_AUTO_TEST_CASE(query_initialization__blocks__verify__expected)
{
    BOOST_REQUIRE_EQUAL(test::block1.hash(), test::block1_hash);
    BOOST_REQUIRE_EQUAL(test::block2.hash(), test::block2_hash);
    BOOST_REQUIRE_EQUAL(test::block3.hash(), test::block3_hash);
    BOOST_REQUIRE_EQUAL(test::block1.header().previous_block_hash(), test::genesis.hash());
    BOOST_REQUIRE_EQUAL(test::block2.header().previous_block_hash(), test::block1.hash());
    BOOST_REQUIRE_EQUAL(test::block3.header().previous_block_hash(), test::block2.hash());
}

// initialize

BOOST_AUTO_TEST_CASE(query_initialization__initialize__is_initialized__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_initialized());
}

// is_initialized

BOOST_AUTO_TEST_CASE(query_initialization__is_initialized__default__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(!query.is_initialized());
}

BOOST_AUTO_TEST_CASE(query_initialization__is_initialized__unconfirmed__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(!query.is_initialized());
}

BOOST_AUTO_TEST_CASE(query_initialization__is_initialized__candidate__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(!query.is_initialized());
}

BOOST_AUTO_TEST_CASE(query_initialization__is_initialized__confirmed__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(!query.is_initialized());
}

BOOST_AUTO_TEST_CASE(query_initialization__is_initialized__candidate_and_confirmed__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.is_initialized());
}

// get_top

BOOST_AUTO_TEST_CASE(query_initialization__get_top__genesis_confirmed__0)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    ////BOOST_REQUIRE(query.push_candidate(query.to_header(genesis.hash())));
    BOOST_REQUIRE_EQUAL(query.get_top_confirmed(), 0u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_top__three_blocks_confirmed__2)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.get_top_confirmed(), 2u);
    BOOST_REQUIRE_EQUAL(query.get_top_candidate(), 0u);
}

// get_top_candidate

BOOST_AUTO_TEST_CASE(query_initialization__get_top_candidate__genesis_candidated__0)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    ////BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE_EQUAL(query.get_top_candidate(), 0u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_top__three_blocks_candidated__2)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.get_top_confirmed(), 0u);
    BOOST_REQUIRE_EQUAL(query.get_top_candidate(), 2u);
}

// get_fork

BOOST_AUTO_TEST_CASE(query_initialization__get_fork__initialized__0)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE_EQUAL(query.get_fork(), 0u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_fork__candidate_ahead__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.get_fork(), 1u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_fork__confirmed_ahead__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.set(test::genesis, test::context));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.get_fork(), 1u);
}

// get_last_associated_from

BOOST_AUTO_TEST_CASE(query_initialization__get_last_associated_from__terminal__max_size_t)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(max_size_t), max_size_t);
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(height_link::terminal), max_size_t);

    // unassociated, but correct.
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(42), 42u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_last_associated_from__initialized__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(0), 0u);

    // unassociated, but correct.
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(42), 42u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_last_associated_from__non_candidate__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(0), 2u);
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(1), 2u);
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(2), 2u);

    // unassociated, but correct.
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(3), 3u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_last_associated_from__gapped_candidate__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2.header(), test::context)); // header only
    BOOST_REQUIRE(query.set(test::block3, test::context));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3.hash())));
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(0), 1u);
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(1), 1u);

    // gapped, but correct.
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(2), 3u);

    // unassociated, but correct.
    BOOST_REQUIRE_EQUAL(query.get_last_associated_from(3), 3u);
}

// get_all_unassociated_above

BOOST_AUTO_TEST_CASE(query_initialization__get_all_unassociated_above__initialized__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.get_all_unassociated_above(0).empty());
    BOOST_REQUIRE(query.get_all_unassociated_above(1).empty());
}

BOOST_AUTO_TEST_CASE(query_initialization__get_all_unassociated_above__gapped_candidate__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2.header(), test::context)); // header only
    BOOST_REQUIRE(query.set(test::block3.header(), test::context)); // header only
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::block3.hash())));
    auto unassociated = query.get_all_unassociated_above(0);
    BOOST_REQUIRE_EQUAL(unassociated.size(), 2u);
    BOOST_REQUIRE_EQUAL(unassociated.front(), test::block2.hash());
    BOOST_REQUIRE_EQUAL(unassociated.back(), test::block3.hash());
    unassociated = query.get_all_unassociated_above(1);
    BOOST_REQUIRE_EQUAL(unassociated.size(), 2u);
    BOOST_REQUIRE_EQUAL(unassociated.front(), test::block2.hash());
    BOOST_REQUIRE_EQUAL(unassociated.back(), test::block3.hash());
    unassociated = query.get_all_unassociated_above(2);
    BOOST_REQUIRE_EQUAL(unassociated.size(), 1u);
    BOOST_REQUIRE_EQUAL(unassociated.back(), test::block3.hash());
    unassociated = query.get_all_unassociated_above(3);
    BOOST_REQUIRE_EQUAL(unassociated.size(), 0u);
}

// get_locator

BOOST_AUTO_TEST_CASE(query_initialization__get_locator__initialized__one)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_locator({ 0, 1, 2, 4, 6, 8 }).size(), 1u);
}

BOOST_AUTO_TEST_CASE(query_initialization__get_locator__gapped__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash())));
    const auto locator = query.get_locator({ 0, 1, 3, 4 });
    BOOST_REQUIRE_EQUAL(locator.size(), 3u);
    BOOST_REQUIRE_EQUAL(locator[0], test::genesis.hash());
    BOOST_REQUIRE_EQUAL(locator[1], test::block1.hash());
    BOOST_REQUIRE_EQUAL(locator[2], test::block3.hash());
}

BOOST_AUTO_TEST_SUITE_END()
