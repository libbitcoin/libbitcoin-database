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

BOOST_FIXTURE_TEST_SUITE(query_pool_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

static transaction to_spend(const system::hash_digest& hash, uint64_t value) NOEXCEPT
{
    const input input{ point{ hash, 0 }, script{}, max_uint32 };
    return { 1u, inputs{ input }, outputs{ output{ value, script{} } }, 0u };
}

static const system::hash_digest& genesis_coinbase() NOEXCEPT
{
    static const auto hash = test::genesis.transactions_ptr()->front()->hash(false);
    return hash;
}

static bool archive_pool(test::query_accessor& query, const transaction& tx) NOEXCEPT
{
    const auto stored = query.set(tx);
    const auto link = query.to_tx(tx.hash(false));
    return stored && query.set_pooled(link, tx, context{ bip113, 8, 9 }, 0);
}

// Confirm a block of a coinbase and the tx above genesis, returning its link.
static header_link archive_confirm(test::query_accessor& query, const transaction& tx) NOEXCEPT
{
    const input null{ point{ system::null_hash, point::null_index }, script{}, max_uint32 };
    const transaction coinbase{ 1u, inputs{ null }, outputs{ output{ 42u, script{} } }, 0u };
    const header header{ 1u, test::genesis.hash(), system::null_hash, 1u, 2u, 3u };
    const block block{ header, { coinbase, tx } };
    const auto stored = query.set(block, context{ 0, 1, 0 }, {}, false, true);
    const auto link = query.to_header(block.hash());
    const auto candidate = stored && query.push_candidate(link);
    const auto confirmed = candidate && query.push_confirmed(link, true);
    return confirmed ? link : header_link{};
}

// get_pooled

BOOST_AUTO_TEST_CASE(query_pool__get_pooled__no_record__unvalidated)
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

BOOST_AUTO_TEST_CASE(query_pool__get_pooled__sufficient__success)
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
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::success);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0x20u);
    BOOST_REQUIRE_EQUAL(pooled.sigops, tx.signature_operations(false, false));
    BOOST_REQUIRE_EQUAL(pooled.prevouts.front().parent, 42u);
    BOOST_REQUIRE(pooled.prevouts.front().coinbase);

    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 9, 10 }), error::success);
}

BOOST_AUTO_TEST_CASE(query_pool__get_pooled__insufficient__unvalidated)
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
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 8 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113 | flags::bip68_rule, 8, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_pooled__without_bip113__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ 0, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ 0, 8, 9 }), error::unvalidated);
}

// get_pooled_time

BOOST_AUTO_TEST_CASE(query_pool__get_pooled_time__pooled__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto tx = to_spend(genesis_coinbase(), 1u);
    BOOST_REQUIRE(query.set(tx));

    const auto link = query.to_tx(tx.hash(false));
    BOOST_REQUIRE(query.set_pooled(link, tx, context{ bip113, 8, 9 }, 0x01020304));

    uint32_t time{};
    BOOST_REQUIRE(query.get_pooled_time(time, link));
    BOOST_REQUIRE_EQUAL(time, 0x01020304u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_pooled_time__not_pooled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t time{};
    BOOST_REQUIRE(!query.get_pooled_time(time, 0));
}

// set_pooled

BOOST_AUTO_TEST_CASE(query_pool__set_pooled__disabled__no_record)
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
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.pool_body_size(), zero);
}

BOOST_AUTO_TEST_CASE(query_pool__set_pooled__time__stored)
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
    BOOST_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0x01020304));

    table::pool::record record{};
    BOOST_REQUIRE(store.pool.get(store.pool.first(tx_link{ 1 }), record));
    BOOST_REQUIRE_EQUAL(record.time, 0x01020304u);
}

BOOST_AUTO_TEST_CASE(query_pool__set_pooled__unlinked_parent__resolved)
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
    BOOST_REQUIRE(query.set_pooled(2, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 2, context{ bip113, 8, 9 }), error::success);
    BOOST_REQUIRE_EQUAL(pooled.prevouts.front().parent, parent);
    BOOST_REQUIRE(!pooled.prevouts.front().coinbase);
}

BOOST_AUTO_TEST_CASE(query_pool__set_pooled__missing_parent__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = max_uint32;
    BOOST_REQUIRE(!query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
}

// is_confirmable_tx

BOOST_AUTO_TEST_CASE(query_pool__is_confirmable_tx__unconflicted_chain__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, parent));
    BOOST_REQUIRE(archive_pool(query, child));
    BOOST_REQUIRE(query.is_confirmable_tx(query.to_tx(parent.hash(false))));
    BOOST_REQUIRE(query.is_confirmable_tx(query.to_tx(child.hash(false))));
}

BOOST_AUTO_TEST_CASE(query_pool__is_confirmable_tx__confirmed__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto tx = to_spend(genesis_coinbase(), 1u);
    BOOST_REQUIRE(archive_pool(query, tx));
    BOOST_REQUIRE(!archive_confirm(query, tx).is_terminal());
    BOOST_REQUIRE(!query.is_confirmable_tx(query.to_tx(tx.hash(false))));
    BOOST_REQUIRE(!query.is_confirmable_tx(tx_link{ 0 }));
}

BOOST_AUTO_TEST_CASE(query_pool__is_confirmable_tx__conflicted_and_child__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, conflicted));
    BOOST_REQUIRE(archive_pool(query, child));
    BOOST_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 1u)).is_terminal());
    BOOST_REQUIRE(!query.is_confirmable_tx(query.to_tx(conflicted.hash(false))));
    BOOST_REQUIRE(!query.is_confirmable_tx(query.to_tx(child.hash(false))));
}

// get_confirmable_txs

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__empty__end)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 0 }, max_size_t), error::success);
    BOOST_REQUIRE(out.empty());
    BOOST_REQUIRE_EQUAL(cursor, 0u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__disabled__end)
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
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, max_size_t), error::success);
    BOOST_REQUIRE(out.empty());
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__cursor_beyond_end__invalid_cursor)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    pool_link cursor{ 3 };
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, max_size_t), error::invalid_cursor);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__confirmed__skipped)
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
    BOOST_REQUIRE(query.set_pooled(0, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, max_size_t), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__all__confirmed_skipped)
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
    BOOST_REQUIRE(query.set_pooled(0, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));

    tx_links out{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__duplicate_record__first_only)
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
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, max_size_t), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__limit__resumed)
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
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));
    BOOST_REQUIRE(query.set_pooled(2, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, one), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 1u);

    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, one), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 2u);
    BOOST_REQUIRE_EQUAL(cursor, 2u);

    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, one), error::success);
    BOOST_REQUIRE(out.empty());
    BOOST_REQUIRE_EQUAL(cursor, 2u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__end__bounded)
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
    BOOST_REQUIRE(query.set_pooled(1, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));
    BOOST_REQUIRE(query.set_pooled(2, test::tx_spend_one_hash, context{ bip113, 8, 9 }, 0));

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 1 }, max_size_t), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), one);
    BOOST_REQUIRE_EQUAL(out.front(), 1u);
    BOOST_REQUIRE_EQUAL(cursor, 1u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_txs__conflicted_and_child__skipped)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, conflicted));
    BOOST_REQUIRE(archive_pool(query, child));
    BOOST_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 1u)).is_terminal());

    tx_links out{};
    pool_link cursor{};
    BOOST_REQUIRE_EQUAL(query.get_confirmable_txs(cursor, out, pool_link{ 2 }, max_size_t), error::success);
    BOOST_REQUIRE(out.empty());
}

// get_confirmable_fees

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_fees__empty__true_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    fee_rates rates{};
    BOOST_CHECK(query.get_confirmable_fees(rates));
    BOOST_CHECK(rates.empty());
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_fees__unconfirmed_and_confirmed__unconfirmed_only)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, context{ 0, 1, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, context{ 0, 2, 0 }, {}, false, false));

    // Block 1 is confirmed (tx 1 is strong), block 2 is not (txs 2, 3, 4).
    BOOST_CHECK(query.push_confirmed(1, false));
    BOOST_CHECK(query.set_strong(1));
    BOOST_CHECK(query.set_strong(2));
    BOOST_CHECK(query.is_confirmed_tx(1));
    BOOST_CHECK(!query.is_confirmed_tx(3));
    BOOST_CHECK(!query.is_confirmed_tx(4));

    BOOST_CHECK(store.pool.put(tx_link{ 1 }, table::pool::record{ {}, { 0, 1, 0 }, 10, {}, {} }));
    BOOST_CHECK(store.pool.put(tx_link{ 3 }, table::pool::record{ {}, { 0, 2, 0 }, 20, {}, {} }));
    BOOST_CHECK(store.pool.put(tx_link{ 4 }, table::pool::record{ {}, { 0, 3, 0 }, 30, {}, {} }));

    fee_rates rates{};
    BOOST_CHECK(query.get_confirmable_fees(rates));
    BOOST_REQUIRE_EQUAL(rates.size(), 2u);
    BOOST_CHECK_EQUAL(rates.at(0).fee, 20u);
    BOOST_CHECK_EQUAL(rates.at(0).height, 2u);
    BOOST_CHECK_EQUAL(rates.at(1).fee, 30u);
    BOOST_CHECK_EQUAL(rates.at(1).height, 3u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_confirmable_fees__conflicted_and_child__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    store.set_pooling();

    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, conflicted));
    BOOST_REQUIRE(archive_pool(query, child));
    BOOST_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());

    fee_rates rates{};
    BOOST_REQUIRE(query.get_confirmable_fees(rates));
    BOOST_REQUIRE(rates.empty());
}

// get_block_conflict_fees

BOOST_AUTO_TEST_CASE(query_pool__get_block_conflict_fees__conflicted_and_child__both)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    store.set_pooling();

    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, conflicted));
    BOOST_REQUIRE(archive_pool(query, child));

    const auto link = archive_confirm(query, to_spend(genesis_coinbase(), 3u));
    BOOST_REQUIRE(!link.is_terminal());

    fee_rates rates{};
    BOOST_REQUIRE(query.get_block_conflict_fees(rates, link));
    BOOST_REQUIRE_EQUAL(rates.size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_pool__get_block_conflict_fees__confirmed_pooled__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    store.set_pooling();

    const auto tx = to_spend(genesis_coinbase(), 1u);
    BOOST_REQUIRE(archive_pool(query, tx));

    const auto link = archive_confirm(query, tx);
    BOOST_REQUIRE(!link.is_terminal());

    fee_rates rates{};
    BOOST_REQUIRE(query.get_block_conflict_fees(rates, link));
    BOOST_REQUIRE(rates.empty());
}

// to_confirmable_parents

BOOST_AUTO_TEST_CASE(query_pool__to_confirmable_parents__pooled_parent__parent)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, parent));
    BOOST_REQUIRE(archive_pool(query, child));

    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    BOOST_REQUIRE(query.to_confirmable_parents(parent_link).empty());
    BOOST_REQUIRE_EQUAL(query.to_confirmable_parents(child_link), tx_links{ parent_link.value });
}

// to_confirmable_children

BOOST_AUTO_TEST_CASE(query_pool__to_confirmable_children__pooled_child__child)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, parent));
    BOOST_REQUIRE(archive_pool(query, child));

    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    BOOST_REQUIRE_EQUAL(query.to_confirmable_children(parent_link), tx_links{ child_link.value });
    BOOST_REQUIRE(query.to_confirmable_children(child_link).empty());
}

BOOST_AUTO_TEST_CASE(query_pool__to_confirmable_children__conflicted_child__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BOOST_REQUIRE(archive_pool(query, parent));
    BOOST_REQUIRE(archive_pool(query, child));
    BOOST_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());

    const auto parent_link = query.to_tx(parent.hash(false));
    BOOST_REQUIRE(query.to_confirmable_children(parent_link).empty());
}

BOOST_AUTO_TEST_SUITE_END()
