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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BC_FIXTURE_TEST_SUITE(query_initialize_tests, test::directory_setup_fixture)

BC_AUTO_TEST_CASE(query_initialize__blocks__verify__expected)
{
    BC_REQUIRE_EQUAL(test::block1.hash(), test::block1_hash);
    BC_REQUIRE_EQUAL(test::block2.hash(), test::block2_hash);
    BC_REQUIRE_EQUAL(test::block3.hash(), test::block3_hash);
    BC_REQUIRE_EQUAL(test::block1.header().previous_block_hash(), test::genesis.hash());
    BC_REQUIRE_EQUAL(test::block2.header().previous_block_hash(), test::block1.hash());
    BC_REQUIRE_EQUAL(test::block3.header().previous_block_hash(), test::block2.hash());
}

// initialize

BC_AUTO_TEST_CASE(query_initialize__initialize__is_initialized__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.is_initialized());
}

// is_initialized

BC_AUTO_TEST_CASE(query_initialize__is_initialized__default__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(!query.is_initialized());
}

BC_AUTO_TEST_CASE(query_initialize__is_initialized__unconfirmed__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BC_REQUIRE(!query.is_initialized());
}

BC_AUTO_TEST_CASE(query_initialize__is_initialized__candidate__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BC_REQUIRE(!query.is_initialized());
}

BC_AUTO_TEST_CASE(query_initialize__is_initialized__confirmed__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    BC_REQUIRE(!query.is_initialized());
}

BC_AUTO_TEST_CASE(query_initialize__is_initialized__candidate_and_confirmed__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    BC_REQUIRE(query.is_initialized());
}

// get_top

BC_AUTO_TEST_CASE(query_initialize__get_top__genesis_confirmed__0)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    ////BC_REQUIRE(query.push_candidate(query.to_header(genesis.hash())));
    BC_REQUIRE_EQUAL(query.get_top_confirmed(), 0u);
}

BC_AUTO_TEST_CASE(query_initialize__get_top__three_blocks_confirmed__2)
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
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BC_REQUIRE_EQUAL(query.get_top_confirmed(), 2u);
    BC_REQUIRE_EQUAL(query.get_top_candidate(), 0u);
}

// get_top_candidate

BC_AUTO_TEST_CASE(query_initialize__get_top_candidate__genesis_candidated__0)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.set(test::genesis, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    ////BC_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash()), false));
    BC_REQUIRE_EQUAL(query.get_top_candidate(), 0u);
}

BC_AUTO_TEST_CASE(query_initialize__get_top__three_blocks_candidated__2)
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
    BC_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BC_REQUIRE_EQUAL(query.get_top_confirmed(), 0u);
    BC_REQUIRE_EQUAL(query.get_top_candidate(), 2u);
}

// get_top_associated_from/get_top_associated

BC_AUTO_TEST_CASE(query_initialize__get_top_associated_from__terminal__max_size_t)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.get_top_associated_from(max_size_t), max_size_t);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(height_link::terminal), max_size_t);
    BC_REQUIRE_EQUAL(query.get_top_associated(), 0u);

    // unassociated, but correct.
    BC_REQUIRE_EQUAL(query.get_top_associated_from(42), 42u);
}

BC_AUTO_TEST_CASE(query_initialize__get_top_associated_from__initialized__zero)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.get_top_associated(), 0u);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(0), 0u);

    // unassociated, but correct.
    BC_REQUIRE_EQUAL(query.get_top_associated_from(42), 42u);
}

BC_AUTO_TEST_CASE(query_initialize__get_top_associated_from__non_candidate__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block3, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BC_REQUIRE_EQUAL(query.get_top_associated(), 2u);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(0), 2u);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(1), 2u);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(2), 2u);

    // unassociated, but correct.
    BC_REQUIRE_EQUAL(query.get_top_associated_from(3), 3u);
}

BC_AUTO_TEST_CASE(query_initialize__get_top_associated_from__gapped_candidate__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2.header(), test::context, {}, false)); // header only
    BC_REQUIRE(query.set(test::block3, test::context, {}, false, false));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block3.hash())));
    BC_REQUIRE_EQUAL(query.get_top_associated(), 1u);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(0), 1u);
    BC_REQUIRE_EQUAL(query.get_top_associated_from(1), 1u);

    // gapped, but correct.
    BC_REQUIRE_EQUAL(query.get_top_associated_from(2), 3u);

    // unassociated, but correct.
    BC_REQUIRE_EQUAL(query.get_top_associated_from(3), 3u);
}

// get_unassociated_above/get_all_unassociated

BC_AUTO_TEST_CASE(query_initialize__get_unassociated_above__initialized__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.get_all_unassociated().empty());
    BC_REQUIRE(query.get_unassociated_above(0).empty());
    BC_REQUIRE(query.get_unassociated_above(1).empty());
}

BC_AUTO_TEST_CASE(query_initialize__get_unassociated_above__gapped_candidate__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));

    constexpr database::context context2
    {
        0x01020302, // flags
        0x00121312, // height (3 bytes)
        0x21222322  // mtp
    };
    constexpr database::context context3
    {
        0x01020303, // flags
        0x00121313, // height (3 bytes)
        0x21222323  // mtp
    };
    BC_REQUIRE(query.initialize(test::genesis));            // associated
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));     // associated
    BC_REQUIRE(query.set(test::block2.header(), context2, {}, false)); // header only
    BC_REQUIRE(query.set(test::block3.header(), context3, {}, false)); // header only
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block3.hash())));

    // There are two unassociated blocks above genesis (fork point).
    BC_REQUIRE_EQUAL(query.get_all_unassociated().size(), 2u);

    const auto unassociated0 = query.get_unassociated_above(0);
    BC_REQUIRE(!unassociated0.empty());
    BC_REQUIRE_EQUAL(unassociated0.size(), 2u);

    const associations copy{ unassociated0.begin(), unassociated0.end() };
    BC_REQUIRE(!copy.empty());
    BC_REQUIRE_EQUAL(copy.size(), 2u);

    BC_REQUIRE(copy.find(system::null_hash) == copy.end());
    BC_REQUIRE(copy.find(system::null_hash) == copy.end());
    BC_REQUIRE(copy.find(zero) == copy.pos_end());

    const auto it2 = unassociated0.find(test::block2.hash());
    BC_REQUIRE(it2 != unassociated0.end());
    BC_REQUIRE_EQUAL(it2->link, 2u);
    BC_REQUIRE_EQUAL(it2->context.flags, context2.flags);
    BC_REQUIRE_EQUAL(it2->context.timestamp, test::block2.header().timestamp());
    BC_REQUIRE_EQUAL(it2->context.median_time_past, context2.mtp);
    BC_REQUIRE_EQUAL(it2->context.height, context2.height);

    const auto it3 = unassociated0.find(test::block3.hash());
    BC_REQUIRE(it3 != unassociated0.end());
    BC_REQUIRE_EQUAL(it3->link, 3u);
    BC_REQUIRE_EQUAL(it3->context.flags, context3.flags);
    BC_REQUIRE_EQUAL(it3->context.timestamp, test::block3.header().timestamp());
    BC_REQUIRE_EQUAL(it3->context.median_time_past, context3.mtp);
    BC_REQUIRE_EQUAL(it3->context.height, context3.height);

    const auto unassociated1 = query.get_unassociated_above(1);
    BC_REQUIRE_EQUAL(unassociated1.size(), 2u);
    BC_REQUIRE(query.get_unassociated_above(1, 0).empty());
    BC_REQUIRE_EQUAL(query.get_unassociated_above(1, 1).size(), 1u);

    const auto it2s = unassociated1.find(context2.height);
    BC_REQUIRE(it2s != unassociated1.pos_end());
    BC_REQUIRE_EQUAL(it2s->context.flags, context2.flags);
    BC_REQUIRE_EQUAL(it2s->context.timestamp, test::block2.header().timestamp());
    BC_REQUIRE_EQUAL(it2s->context.median_time_past, context2.mtp);
    BC_REQUIRE_EQUAL(it2s->context.height, context2.height);

    const auto it3s = unassociated1.find(context3.height);
    BC_REQUIRE(it3s != unassociated1.pos_end());
    BC_REQUIRE_EQUAL(it3s->context.flags, context3.flags);
    BC_REQUIRE_EQUAL(it3s->context.timestamp, test::block3.header().timestamp());
    BC_REQUIRE_EQUAL(it3s->context.median_time_past, context3.mtp);
    BC_REQUIRE_EQUAL(it3s->context.height, context3.height);
    BC_REQUIRE_EQUAL(unassociated1.top().height, context3.height);

    const auto unassociated2 = query.get_unassociated_above(2);
    BC_REQUIRE_EQUAL(unassociated2.size(), 1u);

    const auto it3a = unassociated2.find(test::block3.hash());
    BC_REQUIRE(it3a != unassociated2.end());
    BC_REQUIRE_EQUAL(it3a->context.flags, context3.flags);
    BC_REQUIRE_EQUAL(it3a->context.timestamp, test::block3.header().timestamp());
    BC_REQUIRE_EQUAL(it3a->context.median_time_past, context3.mtp);
    BC_REQUIRE_EQUAL(it3a->context.height, context3.height);
    BC_REQUIRE_EQUAL(unassociated2.top().height, context3.height);

    const auto unassociated3 = query.get_unassociated_above(3);
    BC_REQUIRE_EQUAL(unassociated3.size(), 0u);

    // There are two unassociated blocks above block 1 (new fork point).
    BC_REQUIRE(query.set(test::block1, false, false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block1.hash()), false));
    BC_REQUIRE_EQUAL(query.get_all_unassociated().size(), 2u);

    // There is one unassociated block above block 2 (new fork point).
    BC_REQUIRE(query.set(test::block2, false, false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block2.hash()), false));
    BC_REQUIRE_EQUAL(query.get_all_unassociated().size(), 1u);

    // There are no unassociated blocks above block 3 (new fork point).
    BC_REQUIRE(query.set(test::block3, false, false));
    BC_REQUIRE(query.push_confirmed(query.to_header(test::block3.hash()), false));
    BC_REQUIRE_EQUAL(query.get_all_unassociated().size(), 0u);
}

// associations

static association make_association(size_t height, uint8_t id) NOEXCEPT
{
    association item{};
    item.hash.front() = id;
    item.context.height = height;
    return item;
}

BC_AUTO_TEST_CASE(query_initialize__associations__insert_duplicate__false)
{
    associations map{};
    BC_REQUIRE(map.insert(make_association(1, 0x01)).second);
    BC_REQUIRE(!map.insert(make_association(1, 0x02)).second);
    BC_REQUIRE(!map.insert(make_association(2, 0x01)).second);
    BC_REQUIRE_EQUAL(map.size(), 1u);
    BC_REQUIRE(map.exists(1));
    BC_REQUIRE(!map.exists(2));
}

BC_AUTO_TEST_CASE(query_initialize__associations__pos_begin__height_order)
{
    associations map{};
    BC_REQUIRE(map.insert(make_association(3, 0x03)).second);
    BC_REQUIRE(map.insert(make_association(1, 0x01)).second);
    BC_REQUIRE(map.insert(make_association(2, 0x02)).second);

    auto it = map.pos_begin();
    BC_REQUIRE_EQUAL((it++)->context.height, 1u);
    BC_REQUIRE_EQUAL((it++)->context.height, 2u);
    BC_REQUIRE_EQUAL((it++)->context.height, 3u);
    BC_REQUIRE(it == map.pos_end());
    BC_REQUIRE_EQUAL(map.top().height, 3u);
}

BC_AUTO_TEST_CASE(query_initialize__associations__erase__removed_from_both_indexes)
{
    associations map{};
    const auto item = make_association(1, 0x01);
    BC_REQUIRE(map.insert(item).second);
    BC_REQUIRE(map.insert(make_association(2, 0x02)).second);

    const auto it = map.find(item.hash);
    BC_REQUIRE(it != map.end());
    map.erase(it);
    BC_REQUIRE_EQUAL(map.size(), 1u);
    BC_REQUIRE(!map.exists(item.hash));
    BC_REQUIRE(!map.exists(1));
    BC_REQUIRE(map.insert(item).second);
}

BC_AUTO_TEST_CASE(query_initialize__associations__extract__lowest_moved)
{
    associations map{};
    BC_REQUIRE(map.insert(make_association(3, 0x03)).second);
    BC_REQUIRE(map.insert(make_association(1, 0x01)).second);
    BC_REQUIRE(map.insert(make_association(2, 0x02)).second);

    const auto lowest = map.extract(2);
    BC_REQUIRE_EQUAL(lowest.size(), 2u);
    BC_REQUIRE(lowest.exists(1));
    BC_REQUIRE(lowest.exists(2));
    BC_REQUIRE_EQUAL(map.size(), 1u);
    BC_REQUIRE(map.exists(3));
    BC_REQUIRE(!map.exists(make_association(1, 0x01).hash));
}

BC_AUTO_TEST_CASE(query_initialize__associations__merge__conflicts_retained)
{
    associations map{};
    associations other{};
    BC_REQUIRE(map.insert(make_association(1, 0x01)).second);
    BC_REQUIRE(other.insert(make_association(1, 0x02)).second);
    BC_REQUIRE(other.insert(make_association(2, 0x03)).second);

    map.merge(other);
    BC_REQUIRE_EQUAL(map.size(), 2u);
    BC_REQUIRE(map.exists(2));
    BC_REQUIRE_EQUAL(other.size(), 1u);
    BC_REQUIRE(other.exists(1));
}

BC_AUTO_TEST_CASE(query_initialize__associations__copy__indexes_preserved)
{
    associations map{};
    const auto item = make_association(1, 0x01);
    BC_REQUIRE(map.insert(item).second);

    const auto copy = map;
    map.clear();
    BC_REQUIRE(copy.exists(item.hash));
    BC_REQUIRE(copy.find(item.hash) != copy.end());
    BC_REQUIRE_EQUAL(copy.find(item.hash)->context.height, 1u);
}

// get_unassociated_count_above/get_unassociated_count

BC_AUTO_TEST_CASE(query_initialize__get_unassociated_count_above__gapped_candidate__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));

    constexpr database::context context2
    {
        0x01020302, // flags
        0x00121312, // height (3 bytes)
        0x21222322  // mtp
    };
    constexpr database::context context3
    {
        0x01020303, // flags
        0x00121313, // height (3 bytes)
        0x21222323  // mtp
    };
    BC_REQUIRE(query.initialize(test::genesis));                   // associated
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));     // associated
    BC_REQUIRE(query.set(test::block2.header(), context2, {}, false)); // header only
    BC_REQUIRE(query.set(test::block3.header(), context3, {}, false)); // header only
    BC_REQUIRE(query.push_candidate(query.to_header(test::block1.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block2.hash())));
    BC_REQUIRE(query.push_candidate(query.to_header(test::block3.hash())));

    // There are two unassociated blocks above genesis.
    BC_REQUIRE_EQUAL(query.get_unassociated_count(), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(2), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(3), 0u);

    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0, 0), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0, 1), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0, 2), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0, 3), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1, 0), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1, 1), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1, 2), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1, 3), 2u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(2, 0), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(2, 1), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(2, 2), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(3, 0), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(3, 1), 0u);

    // There is one unassociated block at block 2.
    BC_REQUIRE(query.set(test::block3, false, false));             // associated
    BC_REQUIRE_EQUAL(query.get_unassociated_count(), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1), 1u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(2), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(3), 0u);

    // There are no unassociated blocks.
    BC_REQUIRE(query.set(test::block2, false, false));             // associated
    BC_REQUIRE_EQUAL(query.get_unassociated_count(), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(0), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(1), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(2), 0u);
    BC_REQUIRE_EQUAL(query.get_unassociated_count_above(3), 0u);
}

BC_AUTO_TEST_SUITE_END()
