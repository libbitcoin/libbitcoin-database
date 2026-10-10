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

BOOST_FIXTURE_TEST_SUITE(query_properties_tx_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled__no_row__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0u);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__set_pooled__disabled__no_row)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.pool_body_size(), zero);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled__sufficient__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    const auto& in = *tx.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.parent_tx = 42;
    in.metadata.coinbase = true;
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::success);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0x20u);
    BOOST_REQUIRE_EQUAL(pooled.sigops, tx.signature_operations(false, false));
    BOOST_REQUIRE_EQUAL(pooled.prevouts.front().parent, 42u);
    BOOST_REQUIRE(pooled.prevouts.front().coinbase);

    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 9, 10 }), error::success);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__populate_pooled__sufficient__external_metadata)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction source{ test::tx_spend_one_hash.to_data(true), true };
    const auto& source_in = *source.inputs_ptr()->front();
    source_in.prevout = system::to_shared<output>(0x30, script{});
    source_in.metadata.parent_tx = 42;
    source_in.metadata.coinbase = true;
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 9, 10 }), error::success);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0x20u);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, 42u);
    BOOST_REQUIRE(in.metadata.coinbase);
    BOOST_REQUIRE(!in.prevout);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__populate_pooled__internal_spend__terminal_metadata)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction source{ test::tx_spend_one_hash.to_data(true), true };
    const auto& source_in = *source.inputs_ptr()->front();
    source_in.prevout = system::to_shared<output>(0x30, script{});
    source_in.metadata.parent_tx = 42;
    source_in.metadata.coinbase = true;
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.coinbase = false;

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 8, 9 }), error::success);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
    BOOST_REQUIRE(!in.metadata.coinbase);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__populate_pooled__insufficient__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction source{ test::tx_spend_one_hash.to_data(true), true };
    const auto& source_in = *source.inputs_ptr()->front();
    source_in.prevout = system::to_shared<output>(0x30, script{});
    source_in.metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled__insufficient__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    const auto& in = *tx.inputs_ptr()->front();
    in.metadata.parent_tx = 42;
    in.metadata.coinbase = false;
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 8 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113 | flags::bip68_rule, 8, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0u);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled__without_bip113__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ 0, 8, 9 }));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ 0, 8, 9 }), error::unvalidated);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__set_pooled__unlinked_parent__resolved)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_link parent{};
    BOOST_REQUIRE_EQUAL(query.set_code(parent, test::tx4), error::success);

    const auto& tx = test::tx_spend_tx4;
    tx.inputs_ptr()->front()->metadata.parent_tx = max_uint32;
    BOOST_REQUIRE(query.set_pooled(2, tx, context{ bip113, 8, 9 }));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 2, context{ bip113, 8, 9 }), error::success);
    BOOST_REQUIRE_EQUAL(pooled.prevouts.front().parent, parent);
    BOOST_REQUIRE(!pooled.prevouts.front().coinbase);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__set_pooled__missing_parent__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = max_uint32;
    BOOST_REQUIRE(!query.set_pooled(1, tx, context{ bip113, 8, 9 }));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_wtxid__not_pooled__computed)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_link link{};
    BOOST_REQUIRE(!query.set_code(link, test::tx4));
    BOOST_REQUIRE_EQUAL(query.get_wtxid(link), test::tx4.hash(true));
    BOOST_REQUIRE_NE(query.get_wtxid(link), test::tx4.hash(false));
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_wtxid__pooled__pool_columns)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_link link{};
    BOOST_REQUIRE(!query.set_code(link, test::tx4));

    // Pool another tx under the link, so the result must come from the pool.
    const transaction source{ test::tx5.to_data(true), true };
    source.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(link, source, context{ bip113, 8, 9 }));
    BOOST_REQUIRE_EQUAL(query.get_wtxid(link), test::tx5.hash(true));
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_wtxids__genesis__coinbase_witness_hash)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& coinbase = *test::genesis.transactions_ptr()->front();
    BOOST_REQUIRE_EQUAL(query.get_wtxids(0), system::hashes{ coinbase.hash(true) });
    BOOST_REQUIRE(query.get_wtxids(1).empty());
}

// get_compact_links

constexpr system::siphash_key compact_key
{
    0x0102030405060708_u64,
    0x1112131415161718_u64
};

static uint64_t to_short_id(const transaction& tx) NOEXCEPT
{
    const auto short_hash = system::siphash(compact_key, tx.hash(true));
    return system::bit_and(short_hash, system::unmask_right<uint64_t>(48));
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_compact_links__pooled__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx4{ test::tx4.to_data(true), true };
    const transaction tx5{ test::tx5.to_data(true), true };
    tx4.inputs_ptr()->at(0)->metadata.parent_tx = 42;
    tx4.inputs_ptr()->at(1)->metadata.parent_tx = 42;
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    tx_link link4{};
    tx_link link5{};
    BOOST_REQUIRE(!query.set_code(link4, tx4));
    BOOST_REQUIRE(!query.set_code(link5, tx5));
    BOOST_REQUIRE(query.set_pooled(link4, tx4, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(link5, tx5, context{ bip113, 8, 9 }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5), 0x0000424242424242_u64, to_short_id(tx4) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), 3u);
    BOOST_REQUIRE_EQUAL(out.at(0), link5);
    BOOST_REQUIRE_EQUAL(out.at(1), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(out.at(2), link4);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_compact_links__duplicate_short_id__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx5{ test::tx5.to_data(true), true };
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    tx_link link5{};
    BOOST_REQUIRE(!query.set_code(link5, tx5));
    BOOST_REQUIRE(query.set_pooled(link5, tx5, context{ bip113, 8, 9 }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5), to_short_id(tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(out.at(1), tx_link::terminal);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_compact_links__ambiguous_pool__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx5{ test::tx5.to_data(true), true };
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    // The same wtxid pooled under two distinct links.
    BOOST_REQUIRE(query.set_pooled(1, tx5, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(2, tx5, context{ bip113, 8, 9 }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_compact_links__allocated_row__pooled_matched)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx5{ test::tx5.to_data(true), true };
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    tx_link link5{};
    BOOST_REQUIRE(!query.set_code(link5, tx5));
    BOOST_REQUIRE(query.set_pooled(link5, tx5, context{ bip113, 8, 9 }));

    // A spine row allocated ahead of its columns (a concurrent pool write).
    BOOST_REQUIRE(!store.pool.allocate(1).is_terminal());

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), link5);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_compact_links__uncommitted_row__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Columns written for a row whose spine record is not yet committed.
    using lane_t = schema::pool::witness_lane;
    const auto words = system::from_little_endians(system::array_cast<lane_t>(test::tx5.hash(true)));
    const auto row = store.pool.allocate(1);
    BOOST_REQUIRE(!row.is_terminal());
    BOOST_REQUIRE(store.pool.id0.put(row, table::pool_word{ {}, std::get<0>(words) }));
    BOOST_REQUIRE(store.pool.id1.put(row, table::pool_word{ {}, std::get<1>(words) }));
    BOOST_REQUIRE(store.pool.id2.put(row, table::pool_word{ {}, std::get<2>(words) }));
    BOOST_REQUIRE(store.pool.id3.put(row, table::pool_word{ {}, std::get<3>(words) }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(test::tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_compact_links__disabled__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(test::tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
}

// get_pooled_txs

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled_txs__empty__end)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 0 }, max_size_t), error::success);
    BOOST_REQUIRE(out.empty());
    BOOST_REQUIRE_EQUAL(cursor, 0u);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled_txs__disabled__end)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 2 }, max_size_t), error::success);
    BOOST_REQUIRE(out.empty());
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled_txs__cursor_beyond_end__invalid_cursor)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    pool_link cursor{ 3 };
    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 2 }, max_size_t), error::invalid_cursor);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled_txs__confirmed__skipped)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));

    const auto& in = *test::tx_spend_one_hash.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(0, test::tx_spend_one_hash, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 2 }, max_size_t), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled_txs__limit__resumed)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, {}, false, false));

    const auto& in = *test::tx_spend_one_hash.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(2, test::tx_spend_one_hash, context{ bip113, 8, 9 }));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 2 }, one), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 1u);

    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 2 }, one), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 2u);
    BOOST_REQUIRE_EQUAL(cursor, 2u);

    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 2 }, one), error::success);
    BOOST_REQUIRE(out.empty());
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_pooled_txs__end__bounded)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, {}, false, false));

    const auto& in = *test::tx_spend_one_hash.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(2, test::tx_spend_one_hash, context{ bip113, 8, 9 }));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_pooled_txs(cursor, out, pool_link{ 1 }, max_size_t), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 1u);
}

BOOST_AUTO_TEST_SUITE_END()
