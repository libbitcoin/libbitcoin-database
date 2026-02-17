/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_FIXTURE_TEST_SUITE(query_services_tests, test::directory_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

// Merkle root of test blocks [0..1]
constexpr auto root01 = system::base16_hash("abdc2227d02d114b77be15085c1257709252a7a103f9ac0ab3c85d67e12bc0b8");

// Merkle root of test blocks [2..4]
constexpr auto root02 = system::base16_hash("f2a2a2907abb326726a2d6500fe494f63772a941b414236c302e920bc1aa9caf");

// Merkle root of test blocks [0..4]
constexpr auto root04 = system::sha256::double_hash(root01, root02);

class merkle_accessor
  : public test::query_accessor
{
public:
    using base = test::query_accessor;
    using base::base;
    using base::interval_span;
    using base::create_interval;
    using base::get_confirmed_interval;
    using base::merge_merkle;
    using base::get_merkle_proof;
    using base::get_merkle_tree;
    using base::get_merkle_root_and_proof;
};

// interval_span

BOOST_AUTO_TEST_CASE(query_services__interval_span__uninitialized__max_size_t)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(settings.interval_depth, max_uint8);
    BOOST_REQUIRE_EQUAL(query.interval_span(), max_size_t);
}

BOOST_AUTO_TEST_CASE(query_services__interval_span__11__2048)
{
    settings settings{};
    settings.interval_depth = 11;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.interval_span(), 2048u);
}

BOOST_AUTO_TEST_CASE(query_services__interval_span__0__1)
{
    settings settings{};
    settings.interval_depth = 0;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.interval_span(), 1u);
}

// create_interval

BOOST_AUTO_TEST_CASE(query_services__create_interval__depth_0__block_hash)
{
    settings settings{};
    settings.interval_depth = 0;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    const auto header0 = query.to_header(test::genesis.hash());
    const auto header1 = query.to_header(test::block1.hash());
    const auto header2 = query.to_header(test::block2.hash());
    const auto header3 = query.to_header(test::block3.hash());
    BOOST_REQUIRE(!header0.is_terminal());
    BOOST_REQUIRE(!header1.is_terminal());
    BOOST_REQUIRE(!header2.is_terminal());
    BOOST_REQUIRE(!header3.is_terminal());
    BOOST_REQUIRE(query.create_interval(header0, 0).has_value());
    BOOST_REQUIRE(query.create_interval(header1, 1).has_value());
    BOOST_REQUIRE(query.create_interval(header2, 2).has_value());
    BOOST_REQUIRE(query.create_interval(header3, 3).has_value());
    BOOST_REQUIRE_EQUAL(query.create_interval(header0, 0).value(), test::genesis.hash());
    BOOST_REQUIRE_EQUAL(query.create_interval(header1, 1).value(), test::block1.hash());
    BOOST_REQUIRE_EQUAL(query.create_interval(header2, 2).value(), test::block2.hash());
    BOOST_REQUIRE_EQUAL(query.create_interval(header3, 3).value(), test::block3.hash());
}

BOOST_AUTO_TEST_CASE(query_services__create_interval__depth_1__expected)
{
    settings settings{};
    settings.interval_depth = 1;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    const auto header0 = query.to_header(test::genesis.hash());
    const auto header1 = query.to_header(test::block1.hash());
    const auto header2 = query.to_header(test::block2.hash());
    const auto header3 = query.to_header(test::block3.hash());
    BOOST_REQUIRE(!header0.is_terminal());
    BOOST_REQUIRE(!header1.is_terminal());
    BOOST_REQUIRE(!header2.is_terminal());
    BOOST_REQUIRE(!header3.is_terminal());
    BOOST_REQUIRE(!query.create_interval(header0, 0).has_value());
    BOOST_REQUIRE( query.create_interval(header1, 1).has_value());
    BOOST_REQUIRE(!query.create_interval(header2, 2).has_value());
    BOOST_REQUIRE( query.create_interval(header3, 3).has_value());
    BOOST_REQUIRE_EQUAL(query.create_interval(header1, 1).value(), root01);
    BOOST_REQUIRE_EQUAL(query.create_interval(header3, 3).value(), root02);
}

BOOST_AUTO_TEST_CASE(query_services__create_interval__depth_2__expected)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    const auto header3 = query.to_header(test::block3.hash());
    BOOST_REQUIRE(!header3.is_terminal());
    BOOST_REQUIRE(query.create_interval(header3, 3).has_value());
    BOOST_REQUIRE_EQUAL(query.create_interval(header3, 3).value(), root04);
}

// get_confirmed_interval

BOOST_AUTO_TEST_CASE(query_services__get_confirmed_interval__not_multiple__no_value)
{
    settings settings{};
    settings.interval_depth = 3;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(query.interval_span(), system::power2(settings.interval_depth));
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.get_confirmed_interval(0).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(1).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(6).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(7).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(14).has_value());
}

// Interval is set by create_interval(), integral to set(block).
BOOST_AUTO_TEST_CASE(query_services__get_confirmed_interval__multiple__expected_value)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(query.interval_span(), system::power2(settings.interval_depth));
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));
    BOOST_REQUIRE(!query.get_confirmed_interval(0).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(1).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(2).has_value());
    BOOST_REQUIRE( query.get_confirmed_interval(3).has_value());
    BOOST_REQUIRE(!query.get_confirmed_interval(4).has_value());
}

// merge_merkle

BOOST_AUTO_TEST_CASE(query_services__merge_merkle__empty_from__empty_to)
{
    hashes to{};
    merkle_accessor::merge_merkle(to, {}, 0);
    BOOST_REQUIRE(to.empty());

    merkle_accessor::merge_merkle(to, { system::null_hash }, 0);
    BOOST_REQUIRE(to.empty());
}

BOOST_AUTO_TEST_CASE(query_services__push_merkle__two_leaves_target_zero__merges_one_sibling)
{
    hashes to{};
    hashes from
    {
        test::genesis.hash(),
        test::block1.hash()
    };

    merkle_accessor::merge_merkle(to, std::move(from), 0);
    BOOST_REQUIRE_EQUAL(to.size(), 1u);
    BOOST_REQUIRE_EQUAL(to[0], system::merkle_root({ test::block1.hash() }));
}

BOOST_AUTO_TEST_CASE(query_services__merge_merkle__three_leaves_target_two__handles_odd_length)
{
    hashes to{};
    hashes from
    {
        test::genesis.hash(),
        test::block1.hash(),
        test::block2.hash()
    };

    merkle_accessor::merge_merkle(to, std::move(from), 2);
    BOOST_REQUIRE_EQUAL(to.size(), one);
    BOOST_REQUIRE_EQUAL(to[0], system::merkle_root({ test::genesis.hash(), test::block1.hash() }));
}

BOOST_AUTO_TEST_CASE(query_services__merge_merkle__four_leaves_target_three__merges_two_siblings)
{
    hashes to{};
    hashes from
    {
        test::genesis.hash(),
        test::block1.hash(),
        test::block2.hash(),
        test::block3.hash()
    };

    merkle_accessor::merge_merkle(to, std::move(from), 3);
    BOOST_REQUIRE_EQUAL(to.size(), 2u);
    BOOST_REQUIRE_EQUAL(to[0], system::merkle_root({ test::block2.hash() }));
    BOOST_REQUIRE_EQUAL(to[1], system::merkle_root({ test::genesis.hash(), test::block1.hash() }));
}

// get_merkle_proof

BOOST_AUTO_TEST_CASE(query_services__get_merkle_proof__no_confirmed_blocks__error_merkle_proof)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    hashes proof{};
    BOOST_REQUIRE_EQUAL(query.get_merkle_proof(proof, {}, 5u, 10u), error::merkle_proof);
    BOOST_REQUIRE(proof.empty());
}

BOOST_AUTO_TEST_CASE(query_services__get_merkle_proof__target_in_first_interval__expected)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));

    hashes proof{};
    BOOST_REQUIRE_EQUAL(query.get_merkle_proof(proof, {}, 3u, 3u), error::success);
    BOOST_REQUIRE_EQUAL(proof.size(), 2u);
    BOOST_REQUIRE_EQUAL(proof[0], system::merkle_root({ test::block2.hash() }));
    BOOST_REQUIRE_EQUAL(proof[1], system::merkle_root({ test::genesis.hash(), test::block1.hash() }));
}

BOOST_AUTO_TEST_CASE(query_services__get_merkle_proof__multiple_intervals__expected)
{
    settings settings{};
    settings.interval_depth = 1;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));

    hashes proof{};
    const hashes roots
    {
        system::merkle_root({ test::genesis.hash(), test::block1.hash() }),
        system::merkle_root({ test::block2.hash(), test::block3.hash() })
    };
    BOOST_REQUIRE_EQUAL(query.get_merkle_proof(proof, roots, 3u, 3u), error::success);
    BOOST_REQUIRE_EQUAL(proof.size(), 2u);
    BOOST_REQUIRE_EQUAL(proof[0], system::merkle_root({ test::block2.hash() }));
    BOOST_REQUIRE_EQUAL(proof[1], roots[0]);
}

// get_merkle_tree

BOOST_AUTO_TEST_CASE(query_services__get_merkle_tree__waypoint_zero__genesis)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    hashes tree{};
    const auto expected = test::genesis.hash();
    BOOST_REQUIRE_EQUAL(query.get_merkle_tree(tree, 0), error::success);
    BOOST_REQUIRE_EQUAL(tree.size(), 1u);
    BOOST_REQUIRE_EQUAL(tree[0], expected);
}

BOOST_AUTO_TEST_CASE(query_services__get_merkle_tree__one_full_interval__expected_root)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));

    hashes tree{};
    const auto expected = system::merkle_root(
    {
        test::genesis.hash(),
        test::block1.hash(),
        test::block2.hash(),
        test::block3.hash()
    });
    BOOST_REQUIRE_EQUAL(query.get_merkle_tree(tree, 3), error::success);
    BOOST_REQUIRE_EQUAL(tree.size(), 1u);
    BOOST_REQUIRE_EQUAL(tree[0], expected);
}

// get_merkle_root_and_proof
// get_merkle_proof

BOOST_AUTO_TEST_CASE(query_services__get_merkle_root_and_proof__target_equals_waypoint__success)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));

    // Final root is the merkle root of the entire tree.
    hashes proof{};
    hash_digest root{};
    const auto expected = system::merkle_root(
    {
        test::genesis.hash(),
        test::block1.hash(),
        test::block2.hash(),
        test::block3.hash()
    });
    BOOST_REQUIRE(!query.get_merkle_root_and_proof(root, proof, 3u, 3u));
    BOOST_REQUIRE_EQUAL(proof.size(), 2u);
    BOOST_REQUIRE_EQUAL(proof[0], system::merkle_root({ test::block2.hash() }));
    BOOST_REQUIRE_EQUAL(proof[1], system::merkle_root({ test::genesis.hash(), test::block1.hash() }));
    BOOST_REQUIRE_EQUAL(root, query.get_merkle_root(3));
    BOOST_REQUIRE_EQUAL(root, expected);
}

BOOST_AUTO_TEST_CASE(query_services__get_merkle_root_and_proof__target_less_than_waypoint__success)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));

    // Deeper path because target is not the rightmost.
    hashes proof{};
    hash_digest root{};
    const auto expected = system::merkle_root(
    {
        test::genesis.hash(),
        test::block1.hash(),
        test::block2.hash(),
        test::block3.hash()
    });
    BOOST_REQUIRE(!query.get_merkle_root_and_proof(root, proof, 1u, 3u));
    BOOST_REQUIRE_EQUAL(proof.size(), 2u);
    BOOST_REQUIRE_EQUAL(proof[0], system::merkle_root({ test::genesis.hash() }));
    BOOST_REQUIRE_EQUAL(proof[1], system::merkle_root({ test::block2.hash(), test::block3.hash() }));
    BOOST_REQUIRE_EQUAL(root, query.get_merkle_root(3));
    BOOST_REQUIRE_EQUAL(root, expected);
}

BOOST_AUTO_TEST_CASE(query_services__get_merkle_root_and_proof__target_greater_than_waypoint__error_merkle_arguments)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    hashes proof{};
    hash_digest root{};
    BOOST_REQUIRE_EQUAL(query.get_merkle_root_and_proof(root, proof, 5u, 3u), error::merkle_arguments);
    BOOST_REQUIRE_EQUAL(query.get_merkle_root(3), system::null_hash);
}

BOOST_AUTO_TEST_CASE(query_services__get_merkle_root_and_proof__waypoint_beyond_top__error_merkle_not_found)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    hashes proof{};
    hash_digest root{};
    BOOST_REQUIRE_EQUAL(query.get_merkle_root_and_proof(root, proof, 0u, 100u), error::merkle_not_found);
    BOOST_REQUIRE_EQUAL(query.get_merkle_root(100), system::null_hash);
}

BOOST_AUTO_TEST_SUITE_END()
