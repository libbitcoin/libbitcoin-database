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

BOOST_FIXTURE_TEST_SUITE(query_merkle_tests, test::directory_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

// Example vector from electrumx documentation.
// electrumx.readthedocs.io/en/latest/protocol-methods.html#cp-height-example
//{
//  "branch":
//  [
//     "000000004ebadb55ee9096c9a2f8880e09da59c0d68b1c228da88e48844a1485",
//     "96cbbc84783888e4cc971ae8acf86dd3c1a419370336bb3c634c97695a8c5ac9",
//     "965ac94082cebbcffe458075651e9cc33ce703ab0115c72d9e8b1a9906b2b636",
//     "89e5daa6950b895190716dd26054432b564ccdc2868188ba1da76de8e1dc7591"
//  ],
//  "root  ": "e347b1c43fd9b5415bf0d92708db8284b78daf4d0e24f9c3405f45feb85e25db"
//}

constexpr auto root01 = system::sha256::double_hash(test::block0_hash, test::block1_hash);
constexpr auto root23 = system::sha256::double_hash(test::block2_hash, test::block3_hash);
constexpr auto root03 = system::sha256::double_hash(root01, root23);

constexpr auto root45 = system::sha256::double_hash(test::block4_hash, test::block5_hash);
constexpr auto root67 = system::sha256::double_hash(test::block6_hash, test::block7_hash);
constexpr auto root47 = system::sha256::double_hash(root45, root67);

constexpr auto root07 = system::sha256::double_hash(root03, root47);

constexpr auto root82 = system::sha256::double_hash(test::block8_hash, test::block8_hash);
constexpr auto root84 = system::sha256::double_hash(root82, root82);
constexpr auto root88 = system::sha256::double_hash(root84, root84);

constexpr auto root08 = system::sha256::double_hash(root07, root88);

// depth 1 subroots are just the block hashes.

// depth 1 subroots.
constexpr auto sub01 = system::base16_hash("abdc2227d02d114b77be15085c1257709252a7a103f9ac0ab3c85d67e12bc0b8");
constexpr auto sub23 = system::base16_hash("f2a2a2907abb326726a2d6500fe494f63772a941b414236c302e920bc1aa9caf");
constexpr auto sub45 = system::base16_hash("f9f17a3c6d02b0920eccb11156df370bf4117fae2233dfee40817586ba981ca5");
constexpr auto sub67 = system::base16_hash("96cbbc84783888e4cc971ae8acf86dd3c1a419370336bb3c634c97695a8c5ac9");
constexpr auto sub82 = system::base16_hash("67552d97dfd80082ecd5fe3b233e3a4aa9cb9a07a6040bb43b507cbec44088f2");
static_assert(sub01 == root01);
static_assert(sub23 == root23);
static_assert(sub45 == root45);
static_assert(sub67 == root67);
static_assert(sub82 == root82);

// depth 2 subroots.
constexpr auto sub03 = system::base16_hash("965ac94082cebbcffe458075651e9cc33ce703ab0115c72d9e8b1a9906b2b636");
constexpr auto sub47 = system::base16_hash("0e85585b6afb71116ec439b72a25edb8003ef34bc42fb2c88a05249da335774d");
constexpr auto sub84 = system::base16_hash("c752fe3464335530a1109a7cfc6193f9aafb6d0dd913a4a51b92bc6cc4a90c33");
static_assert(sub03 == root03);
static_assert(sub47 == root47);
static_assert(sub84 == root84);

// depth 3 subroots.
constexpr auto sub07 = system::base16_hash("c809e7a698a4b4c474ff6f5f05e88af6d7cb80ddbbe302660dfe6bd1969224a2");
constexpr auto sub88 = system::base16_hash("89e5daa6950b895190716dd26054432b564ccdc2868188ba1da76de8e1dc7591");
static_assert(sub07 == root07);
static_assert(sub88 == root88);

// depth 4 root (is not a subroot)
constexpr auto span08 = system::base16_hash("e347b1c43fd9b5415bf0d92708db8284b78daf4d0e24f9c3405f45feb85e25db");
static_assert(span08 == root08);

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
    using base::get_merkle_subroots;
    using base::get_merkle_root_and_proof;
};

// interval_span

BOOST_AUTO_TEST_CASE(query_merkle__interval_span__uninitialized__max_size_t)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(settings.interval_depth, max_uint8);
    BOOST_CHECK_EQUAL(query.interval_span(), max_size_t);
}

BOOST_AUTO_TEST_CASE(query_merkle__interval_span__11__2048)
{
    settings settings{};
    settings.interval_depth = 11;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.interval_span(), 2048u);
}

BOOST_AUTO_TEST_CASE(query_merkle__interval_span__0__1)
{
    settings settings{};
    settings.interval_depth = 0;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.interval_span(), 1u);
}

// create_interval

BOOST_AUTO_TEST_CASE(query_merkle__create_interval__depth_0__block_hash)
{
    settings settings{};
    settings.interval_depth = 0;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    const auto header0 = query.to_header(test::block0_hash);
    const auto header1 = query.to_header(test::block1_hash);
    const auto header2 = query.to_header(test::block2_hash);
    const auto header3 = query.to_header(test::block3_hash);
    BOOST_CHECK(!header0.is_terminal());
    BOOST_CHECK(!header1.is_terminal());
    BOOST_CHECK(!header2.is_terminal());
    BOOST_CHECK(!header3.is_terminal());
    BOOST_CHECK(query.create_interval(header0, 0).has_value());
    BOOST_CHECK(query.create_interval(header1, 1).has_value());
    BOOST_CHECK(query.create_interval(header2, 2).has_value());
    BOOST_CHECK(query.create_interval(header3, 3).has_value());
    BOOST_CHECK_EQUAL(query.create_interval(header0, 0).value(), test::block0_hash);
    BOOST_CHECK_EQUAL(query.create_interval(header1, 1).value(), test::block1_hash);
    BOOST_CHECK_EQUAL(query.create_interval(header2, 2).value(), test::block2_hash);
    BOOST_CHECK_EQUAL(query.create_interval(header3, 3).value(), test::block3_hash);
}

BOOST_AUTO_TEST_CASE(query_merkle__create_interval__depth_1__expected)
{
    settings settings{};
    settings.interval_depth = 1;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    const auto header0 = query.to_header(test::block0_hash);
    const auto header1 = query.to_header(test::block1_hash);
    const auto header2 = query.to_header(test::block2_hash);
    const auto header3 = query.to_header(test::block3_hash);
    BOOST_CHECK(!header0.is_terminal());
    BOOST_CHECK(!header1.is_terminal());
    BOOST_CHECK(!header2.is_terminal());
    BOOST_CHECK(!header3.is_terminal());
    BOOST_CHECK(!query.create_interval(header0, 0).has_value());
    BOOST_CHECK( query.create_interval(header1, 1).has_value());
    BOOST_CHECK(!query.create_interval(header2, 2).has_value());
    BOOST_CHECK( query.create_interval(header3, 3).has_value());
    BOOST_CHECK_EQUAL(query.create_interval(header1, 1).value(), root01);
    BOOST_CHECK_EQUAL(query.create_interval(header3, 3).value(), root23);
}

BOOST_AUTO_TEST_CASE(query_merkle__create_interval__depth_2__expected)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));

    const auto header3 = query.to_header(test::block3_hash);
    BOOST_CHECK(!header3.is_terminal());
    BOOST_CHECK(query.create_interval(header3, 3).has_value());
    BOOST_CHECK_EQUAL(query.create_interval(header3, 3).value(), root03);
}

// get_confirmed_interval

BOOST_AUTO_TEST_CASE(query_merkle__get_confirmed_interval__not_multiple__no_value)
{
    settings settings{};
    settings.interval_depth = 3;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(query.interval_span(), system::power2(settings.interval_depth));
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(!query.get_confirmed_interval(0).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(1).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(6).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(7).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(14).has_value());
}

// Interval is set by create_interval(), integral to set(block).
BOOST_AUTO_TEST_CASE(query_merkle__get_confirmed_interval__multiple__expected_value)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(query.interval_span(), system::power2(settings.interval_depth));
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));
    BOOST_CHECK(!query.get_confirmed_interval(0).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(1).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(2).has_value());
    BOOST_CHECK( query.get_confirmed_interval(3).has_value());
    BOOST_CHECK(!query.get_confirmed_interval(4).has_value());
}

// merge_merkle

BOOST_AUTO_TEST_CASE(query_merkle__merge_merkle__empty_from__expected)
{
    hashes to{};
    merkle_accessor::merge_merkle(to, {}, 0, 0);
    BOOST_CHECK(to.empty());
}

BOOST_AUTO_TEST_CASE(query_merkle__merge_merkle__one_hash__empty)
{
    hashes to{};
    merkle_accessor::merge_merkle(to, { system::null_hash }, 0, 0);
    BOOST_CHECK(to.empty());
}

BOOST_AUTO_TEST_CASE(query_merkle__push_merkle__two_leaves_target_zero__expected)
{
    hashes to{};
    hashes from
    {
        test::block0_hash,
        test::block1_hash
    };

    merkle_accessor::merge_merkle(to, std::move(from), 0, 0);
    BOOST_CHECK_EQUAL(to.size(), 1u);
    BOOST_CHECK_EQUAL(to[0], test::block1_hash);
}

BOOST_AUTO_TEST_CASE(query_merkle__merge_merkle__three_leaves_target_two__expected)
{
    hashes to{};
    hashes from
    {
        test::block0_hash,
        test::block1_hash,
        test::block2_hash
    };

    merkle_accessor::merge_merkle(to, std::move(from), 2, 0);
    BOOST_CHECK_EQUAL(to.size(), 2u);
    BOOST_CHECK_EQUAL(to[0], test::block2_hash);
    BOOST_CHECK_EQUAL(to[1], root01);
}

BOOST_AUTO_TEST_CASE(query_merkle__merge_merkle__four_leaves_target_three__expected)
{
    hashes to{};
    hashes from
    {
        test::block0_hash,
        test::block1_hash,
        test::block2_hash,
        test::block3_hash
    };

    merkle_accessor::merge_merkle(to, std::move(from), 3, 0);
    BOOST_CHECK_EQUAL(to.size(), 2u);
    BOOST_CHECK_EQUAL(to[0], test::block2_hash);
    BOOST_CHECK_EQUAL(to[1], root01);
}

// get_merkle_proof

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_proof__no_confirmed_blocks__error_merkle_proof)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));

    hashes proof{};
    BOOST_CHECK_EQUAL(query.get_merkle_proof(proof, {}, 5, 10), error::merkle_proof);
    BOOST_CHECK(proof.empty());
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_proof__target_in_first_interval__expected)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));

    hashes proof{};
    BOOST_CHECK_EQUAL(query.get_merkle_proof(proof, {}, 3u, 3u), error::success);
    BOOST_CHECK_EQUAL(proof.size(), 2u);
    BOOST_CHECK_EQUAL(proof[0], test::block2_hash);
    BOOST_CHECK_EQUAL(proof[1], root01);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_proof__multiple_intervals__expected)
{
    settings settings{};
    settings.interval_depth = 1;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));

    hashes proof{};
    const hashes roots
    {
        root01,
        root23
    };
    BOOST_CHECK_EQUAL(query.get_merkle_proof(proof, roots, 3u, 3u), error::success);
    BOOST_CHECK_EQUAL(proof.size(), 2u);
    BOOST_CHECK_EQUAL(proof[0], test::block2_hash);
    BOOST_CHECK_EQUAL(proof[1], root01);
}

// get_merkle_subroots

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_subroots__waypoint_zero__genesis)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 0), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 1u);

    // At depth 2, the 1st position (block 0) the hash is the root.
    BOOST_CHECK_EQUAL(roots[0], test::block0_hash);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_subroots__one_full_interval__expected_root)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 3), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 1u);

    // At depth 2, the 4th position (block 3) results in an interval subroot as the root.
    BOOST_CHECK_EQUAL(roots[0], root03);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_subroots__full_and_partial_interval__expected_two_roots)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.set(test::block4, context{ 0, 4, 0 }, false, false));
    BOOST_CHECK(query.set(test::block5, context{ 0, 5, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block4_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block5_hash), false));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 5), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 2u);
    BOOST_CHECK_EQUAL(roots[0], root03);

    // At depth 2, the 6th position (block 5) results in one complete and one partial root.
    constexpr auto root45 = system::sha256::double_hash(test::block4_hash, test::block5_hash);
    constexpr auto root4545 = system::sha256::double_hash(root45, root45);
    BOOST_CHECK_EQUAL(roots[1], root4545);
}

// get_merkle_root_and_proof
// get_merkle_proof

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__target_equals_waypoint__success)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 3, 3));
    BOOST_CHECK_EQUAL(proof.size(), 2u);
    BOOST_CHECK_EQUAL(proof[0], test::block2_hash);
    BOOST_CHECK_EQUAL(proof[1], root01);
    BOOST_CHECK_EQUAL(root, query.get_merkle_root(3));
    BOOST_CHECK_EQUAL(root, root03);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__target_less_than_waypoint__success)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block1_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block2_hash), false));
    BOOST_CHECK(query.push_confirmed(query.to_header(test::block3_hash), false));

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 1, 3));
    BOOST_CHECK_EQUAL(proof.size(), 2u);
    BOOST_CHECK_EQUAL(proof[0], test::block0_hash);
    BOOST_CHECK_EQUAL(proof[1], root23);
    BOOST_CHECK_EQUAL(root, query.get_merkle_root(3));
    BOOST_CHECK_EQUAL(root, root03);
}

bool setup_eight_block_store(merkle_accessor& query)
{
    return query.initialize(test::genesis) &&
        query.set(test::block1, context{ 0, 1, 0 }, false, false) &&
        query.set(test::block2, context{ 0, 2, 0 }, false, false) &&
        query.set(test::block3, context{ 0, 3, 0 }, false, false) &&
        query.set(test::block4, context{ 0, 4, 0 }, false, false) &&
        query.set(test::block5, context{ 0, 5, 0 }, false, false) &&
        query.set(test::block6, context{ 0, 6, 0 }, false, false) &&
        query.set(test::block7, context{ 0, 7, 0 }, false, false) &&
        query.set(test::block8, context{ 0, 8, 0 }, false, false) &&
        query.push_confirmed(query.to_header(test::block1_hash), false) &&
        query.push_confirmed(query.to_header(test::block2_hash), false) &&
        query.push_confirmed(query.to_header(test::block3_hash), false) &&
        query.push_confirmed(query.to_header(test::block4_hash), false) &&
        query.push_confirmed(query.to_header(test::block5_hash), false) &&
        query.push_confirmed(query.to_header(test::block6_hash), false) &&
        query.push_confirmed(query.to_header(test::block7_hash), false) &&
        query.push_confirmed(query.to_header(test::block8_hash), false);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__electrumx_example_depth_0__success)
{
    settings settings{};
    settings.interval_depth = 0;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 9u);
    BOOST_CHECK_EQUAL(roots[0], test::block0_hash);
    BOOST_CHECK_EQUAL(roots[1], test::block1_hash);
    BOOST_CHECK_EQUAL(roots[2], test::block2_hash);
    BOOST_CHECK_EQUAL(roots[3], test::block3_hash);
    BOOST_CHECK_EQUAL(roots[4], test::block4_hash);
    BOOST_CHECK_EQUAL(roots[5], test::block5_hash);
    BOOST_CHECK_EQUAL(roots[6], test::block6_hash);
    BOOST_CHECK_EQUAL(roots[7], test::block7_hash);
    BOOST_CHECK_EQUAL(roots[8], test::block8_hash);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 5, 8));
    BOOST_CHECK_EQUAL(root, root08);
    BOOST_CHECK_EQUAL(proof.size(), 4u);
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], sub67);
    BOOST_CHECK_EQUAL(proof[2], sub03);
    BOOST_CHECK_EQUAL(proof[3], sub88);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__electrumx_example_depth_1__success)
{
    settings settings{};
    settings.interval_depth = 1;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 5u);
    BOOST_CHECK_EQUAL(roots[0], sub01);
    BOOST_CHECK_EQUAL(roots[1], sub23);
    BOOST_CHECK_EQUAL(roots[2], sub45);
    BOOST_CHECK_EQUAL(roots[3], sub67);
    BOOST_CHECK_EQUAL(roots[4], sub82);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 5, 8));
    BOOST_CHECK_EQUAL(root, span08);
    BOOST_CHECK_EQUAL(proof.size(), 4u);
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], sub67);
    BOOST_CHECK_EQUAL(proof[2], sub03);
    BOOST_CHECK_EQUAL(proof[3], sub88);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__electrumx_example_depth_2__success)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 3u);
    BOOST_CHECK_EQUAL(roots[0], sub03);
    BOOST_CHECK_EQUAL(roots[1], sub47);
    BOOST_CHECK_EQUAL(roots[2], sub84);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 5, 8));
    BOOST_CHECK_EQUAL(root, span08);
    BOOST_CHECK_EQUAL(proof.size(), 4u);
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], sub67);
    BOOST_CHECK_EQUAL(proof[2], sub03);
    BOOST_CHECK_EQUAL(proof[3], sub88);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__electrumx_example_depth_3__success)
{
    settings settings{};
    settings.interval_depth = 3;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 2u);
    BOOST_CHECK_EQUAL(roots[0], sub07);
    BOOST_CHECK_EQUAL(roots[1], sub88);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 5, 8));
    BOOST_CHECK_EQUAL(root, span08);
    BOOST_CHECK_EQUAL(proof.size(), 4u);
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], sub67);
    BOOST_CHECK_EQUAL(proof[2], sub03);
    BOOST_CHECK_EQUAL(proof[3], sub88);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__electrumx_example_depth_4__success)
{
    settings settings{};
    settings.interval_depth = 4;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 1u);
    BOOST_CHECK_EQUAL(roots[0], span08);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 5, 8));
    BOOST_CHECK_EQUAL(root, span08);
    BOOST_CHECK_EQUAL(proof.size(), 4u); // <<<<< FAIL (proof.size() == 5)
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], sub67);
    BOOST_CHECK_EQUAL(proof[2], sub03);
    BOOST_CHECK_EQUAL(proof[3], sub88);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__electrumx_example_depth_11__success)
{
    settings settings{};
    settings.interval_depth = 11;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 1u);
    BOOST_CHECK_EQUAL(roots[0], span08);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 5, 8));
    BOOST_CHECK_EQUAL(root, span08);
    BOOST_CHECK_EQUAL(proof.size(), 4u); // <<<<< FAIL (proof.size() == 11)
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], sub67);
    BOOST_CHECK_EQUAL(proof[2], sub03);
    BOOST_CHECK_EQUAL(proof[3], sub88);
}

// This tests a potentially sparse path (avoids compression).
BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__target_8_depth_0__success)
{
    settings settings{};
    settings.interval_depth = 0;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 8), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 9u);
    BOOST_CHECK_EQUAL(roots[0], test::block0_hash);
    BOOST_CHECK_EQUAL(roots[1], test::block1_hash);
    BOOST_CHECK_EQUAL(roots[2], test::block2_hash);
    BOOST_CHECK_EQUAL(roots[3], test::block3_hash);
    BOOST_CHECK_EQUAL(roots[4], test::block4_hash);
    BOOST_CHECK_EQUAL(roots[5], test::block5_hash);
    BOOST_CHECK_EQUAL(roots[6], test::block6_hash);
    BOOST_CHECK_EQUAL(roots[7], test::block7_hash);
    BOOST_CHECK_EQUAL(roots[8], test::block8_hash);

    BOOST_CHECK_EQUAL(query.get_merkle_root(8), span08);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 8, 8));
    BOOST_CHECK_EQUAL(root, span08);
    BOOST_CHECK_EQUAL(proof.size(), 4u);
    BOOST_CHECK_EQUAL(proof[0], test::block8_hash);
    BOOST_CHECK_EQUAL(proof[1], root82);
    BOOST_CHECK_EQUAL(proof[2], root84);
    BOOST_CHECK_EQUAL(proof[3], root07);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__partial_interval_with_elevation_depth__success)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(setup_eight_block_store(query));

    constexpr auto root42 = system::sha256::double_hash(test::block4_hash, test::block4_hash);
    constexpr auto root44 = system::sha256::double_hash(root42, root42);
    constexpr auto expected_root = system::sha256::double_hash(root03, root44);

    hashes roots{};
    BOOST_CHECK_EQUAL(query.get_merkle_subroots(roots, 4), error::success);
    BOOST_CHECK_EQUAL(roots.size(), 2u);
    BOOST_CHECK_EQUAL(roots[0], root03);
    BOOST_CHECK_EQUAL(roots[1], root44);

    BOOST_CHECK_EQUAL(query.get_merkle_root(4), expected_root);

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK(!query.get_merkle_root_and_proof(root, proof, 4, 4));
    BOOST_CHECK_EQUAL(root, expected_root);

    // expected result:
    BOOST_CHECK_EQUAL(proof.size(), 3u);
    BOOST_CHECK_EQUAL(proof[0], test::block4_hash);
    BOOST_CHECK_EQUAL(proof[1], root42);
    BOOST_CHECK_EQUAL(proof[2], root03);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__target_greater_than_waypoint__error_invalid_argument)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK_EQUAL(query.get_merkle_root_and_proof(root, proof, 5, 3), error::invalid_argument);
    BOOST_CHECK_EQUAL(query.get_merkle_root(3), system::null_hash);
}

BOOST_AUTO_TEST_CASE(query_merkle__get_merkle_root_and_proof__waypoint_beyond_top__error_not_found)
{
    settings settings{};
    settings.interval_depth = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    merkle_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));

    hashes proof{};
    hash_digest root{};
    BOOST_CHECK_EQUAL(query.get_merkle_root_and_proof(root, proof, 0, 100), error::not_found);
    BOOST_CHECK_EQUAL(query.get_merkle_root(100), system::null_hash);
}

BOOST_AUTO_TEST_SUITE_END()

// ==================================
// config   : 0
// span     : 1
// leaves   : 9
// waypoint : 8
// reserve  : 10
// A cached subroot : 0 - 0 [000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f]
// A cached subroot : 1 - 1 [00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048]
// A cached subroot : 2 - 2 [000000006a625f06636b8bb6ac7b960a8d03705d1ace08b1a19da3fdcc99ddbd]
// A cached subroot : 3 - 3 [0000000082b5015589a3fdf2d4baff403e6f0be035a5d9742c1cae6295464449]
// A cached subroot : 4 - 4 [000000004ebadb55ee9096c9a2f8880e09da59c0d68b1c228da88e48844a1485]
// A cached subroot : 5 - 5 [000000009b7262315dbf071787ad3656097b892abffd1f95a1a022f896f533fc]
// A cached subroot : 6 - 6 [000000003031a0e73735690c5a1ff2a4be82553b2a12b776fbd3a215dc8f778d]
// A cached subroot : 7 - 7 [0000000071966c2b1d065fd446b1e485b2c9d9594acd2007ccbd5441cfc89444]
// A cached subroot : 8 - 8 [00000000408c48f847aa786c2268fc3e6ec2af68e8468a34a28c61b7f1de0dc6]
// ==================================
// config   : 1
// span     : 2
// leaves   : 9
// waypoint : 8
// reserve  : 6
// A cached subroot : 0 - 1 [abdc2227d02d114b77be15085c1257709252a7a103f9ac0ab3c85d67e12bc0b8]
// A cached subroot : 2 - 3 [f2a2a2907abb326726a2d6500fe494f63772a941b414236c302e920bc1aa9caf]
// A cached subroot : 4 - 5 [f9f17a3c6d02b0920eccb11156df370bf4117fae2233dfee40817586ba981ca5]
// A cached subroot : 6 - 7 [96cbbc84783888e4cc971ae8acf86dd3c1a419370336bb3c634c97695a8c5ac9]
// depth    : 1
// evened   : 2
// merkled  : 2
// Computed subroot : 8 - 8 [67552d97dfd80082ecd5fe3b233e3a4aa9cb9a07a6040bb43b507cbec44088f2]
// ==================================
// config   : 2
// span     : 4
// leaves   : 9
// waypoint : 8
// reserve  : 4
// A cached subroot : 0 - 3 [965ac94082cebbcffe458075651e9cc33ce703ab0115c72d9e8b1a9906b2b636]
// A cached subroot : 4 - 7 [0e85585b6afb71116ec439b72a25edb8003ef34bc42fb2c88a05249da335774d]
// depth    : 2
// evened   : 2
// merkled  : 2
// hashed   : 1
// Computed subroot : 8 - 8 [c752fe3464335530a1109a7cfc6193f9aafb6d0dd913a4a51b92bc6cc4a90c33]
// ==================================
// config   : 3
// span     : 8
// leaves   : 9
// waypoint : 8
// reserve  : 2
// A cached subroot : 0 - 7 [c809e7a698a4b4c474ff6f5f05e88af6d7cb80ddbbe302660dfe6bd1969224a2]
// depth    : 3
// evened   : 2
// merkled  : 2
// hashed   : 1
// hashed   : 2
// Computed subroot : 8 - 8 [89e5daa6950b895190716dd26054432b564ccdc2868188ba1da76de8e1dc7591]
// ==================================
// config   : 4
// span     : 16
// leaves   : 9
// waypoint : 8
// reserve  : 1
// The merkle root  : 0 - 8 [e347b1c43fd9b5415bf0d92708db8284b78daf4d0e24f9c3405f45feb85e25db]
// ==================================
// config   : 11
// span     : 2048
// leaves   : 9
// waypoint : 8
// reserve  : 1
// The merkle root  : 0 - 8 [e347b1c43fd9b5415bf0d92708db8284b78daf4d0e24f9c3405f45feb85e25db]
