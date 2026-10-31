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

BC_FIXTURE_TEST_SUITE(query_navigate_tests, test::directory_setup_fixture)

// to_candidate

BC_AUTO_TEST_CASE(query_navigate__to_candidate__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));

    // initialize pushes the genesis candidate. 
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BC_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BC_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);

    // key-link translate of actual candidates.
    BC_REQUIRE(query.push_candidate(1));
    BC_REQUIRE(query.push_candidate(2));
    BC_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BC_REQUIRE_EQUAL(query.to_candidate(1), 1u);
    BC_REQUIRE_EQUAL(query.to_candidate(2), 2u);
    BC_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BC_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
}

// to_confirmed

BC_AUTO_TEST_CASE(query_navigate__to_confirmed__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));

    // initialize pushes the genesis confirmed. 
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BC_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BC_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);

    // key-link translate of actual confirmeds.
    BC_REQUIRE(query.push_confirmed(1, false));
    BC_REQUIRE(query.push_confirmed(2, false));
    BC_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BC_REQUIRE_EQUAL(query.to_confirmed(1), 1u);
    BC_REQUIRE_EQUAL(query.to_confirmed(2), 2u);
    BC_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BC_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BC_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
}

// to_top_candidate

BC_AUTO_TEST_CASE(query_navigate__to_top_candidate__push_pop__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 0u);
    BC_REQUIRE(query.push_candidate(1));
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 1u);
    BC_REQUIRE(query.push_candidate(2));
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 2u);
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 0u);
    BC_REQUIRE(query.pop_candidate());
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 1u);
    BC_REQUIRE(query.pop_candidate());
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 0u);
    BC_REQUIRE(!query.pop_candidate());
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 0u);
}

// to_top_confirmed

BC_AUTO_TEST_CASE(query_navigate__to_top_confirmed__push_pop__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 0u);
    BC_REQUIRE(query.push_confirmed(1, false));
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 1u);
    BC_REQUIRE(query.push_confirmed(2, false));
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 2u);
    BC_REQUIRE_EQUAL(query.to_top_candidate(), 0u);
    BC_REQUIRE(query.pop_confirmed());
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 1u);
    BC_REQUIRE(query.pop_confirmed());
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 0u);
    BC_REQUIRE(!query.pop_confirmed());
    BC_REQUIRE_EQUAL(query.to_top_confirmed(), 0u);
}

// to_header

BC_AUTO_TEST_CASE(query_navigate__to_header__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    header_link link{};

    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), header_link::terminal);
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), 0u);
    BC_REQUIRE_EQUAL(query.to_header(test::block1.hash()), header_link::terminal);
    BC_REQUIRE_EQUAL(query.set_code(link, test::block1.header(), test::context, {}, false), error::success);
    BC_REQUIRE_EQUAL(link, 1u);
    BC_REQUIRE_EQUAL(query.to_header(test::block1.hash()), 1u);
}

// to_tx

BC_AUTO_TEST_CASE(query_navigate__to_witness_tx__unsegregated__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));

    // An unsegregated tx has the same hash with and without witness.
    BC_REQUIRE_EQUAL(query.to_witness_tx(test::genesis.transactions_ptr()->front()->hash(true)), 0u);
    BC_REQUIRE_EQUAL(query.to_witness_tx(test::block1.transactions_ptr()->front()->hash(true)), 1u);
    BC_REQUIRE(query.to_witness_tx(system::one_hash).is_terminal());
}

BC_AUTO_TEST_CASE(query_navigate__to_witness_tx__segregated_pooled__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    using namespace system::chain;
    const point prevout{ test::genesis.transactions_ptr()->front()->hash(false), 0 };
    const input in{ prevout, script{}, witness{ system::data_stack{ system::data_chunk{ 0x42 } } }, max_uint32 };
    const transaction tx{ 2u, inputs{ in }, outputs{ output{ 1u, script{} } }, 0u };
    tx.inputs_ptr()->front()->prevout = system::to_shared<output>(2u, script{});
    BC_REQUIRE(tx.is_segregated());
    BC_REQUIRE(tx.hash(true) != tx.hash(false));
    BC_REQUIRE(query.set(tx));

    const auto link = query.to_tx(tx.hash(false));
    BC_REQUIRE(query.to_witness_tx(tx.hash(true)).is_terminal());
    BC_REQUIRE(query.set_pooled(link, tx, database::context{ 0, 1, 0 }, 0));
    BC_REQUIRE(query.is_pooled(link));
    BC_REQUIRE_EQUAL(query.to_witness_tx(tx.hash(true)), link);
    BC_REQUIRE(query.to_witness_tx(tx.hash(false)).is_terminal());
    BC_REQUIRE_EQUAL(query.wtxid_records(), query.pool_records());
}

BC_AUTO_TEST_CASE(query_navigate__to_tx__transactions__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));

    // All four blocks have one transaction.
    BC_REQUIRE_EQUAL(query.to_tx(test::genesis.transactions_ptr()->front()->hash(true)), 0u);
    BC_REQUIRE_EQUAL(query.to_tx(test::block1.transactions_ptr()->front()->hash(true)), 1u);
    BC_REQUIRE_EQUAL(query.to_tx(test::block2.transactions_ptr()->front()->hash(true)), 2u);
    BC_REQUIRE_EQUAL(query.to_tx(test::block3.transactions_ptr()->front()->hash(true)), tx_link::terminal);
}

// to_filter
// get_header_key

BC_AUTO_TEST_CASE(query_navigate__get_header_key__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK_EQUAL(query.get_header_key(0), test::genesis.hash());
    BC_CHECK_EQUAL(query.get_header_key(1), system::null_hash);
}

// get_point_key

BC_AUTO_TEST_CASE(query_navigate__get_point_key__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK_EQUAL(query.get_point_hash(0), system::null_hash);

    // tx4/5 prevouts are all block1a.tx1.
    BC_CHECK(query.set(test::tx4));
    BC_CHECK(query.set(test::tx5));
    BC_CHECK_EQUAL(query.get_point_hash(1), test::block1a.transactions_ptr()->front()->hash(false));
    BC_CHECK_EQUAL(query.get_point_hash(2), test::block1a.transactions_ptr()->front()->hash(false));

    // block1a adds three prevouts of two txs.
    BC_CHECK(query.set(test::block1a, context{}, {}, false, false));
    BC_CHECK_EQUAL(query.get_point_hash(4), system::one_hash);
}

// get_outs_key

BC_AUTO_TEST_CASE(query_navigate__get_outs_key__genesis__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK_EQUAL(query.get_outs_key(0), test::genesis.transactions_ptr()->front()->outputs_ptr()->front()->script().hash());
}

// get_tx_key

BC_AUTO_TEST_CASE(query_navigate__get_tx_key__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK_EQUAL(query.get_tx_key(0), test::genesis.transactions_ptr()->front()->hash(false));
    BC_CHECK_EQUAL(query.get_tx_key(1), system::null_hash);
    BC_CHECK(query.set(test::tx4));
    BC_CHECK(query.set(test::tx5));
    BC_CHECK_EQUAL(query.get_tx_key(1), test::tx4.hash(false));
    BC_CHECK_EQUAL(query.get_tx_key(2), test::tx5.hash(false));
    BC_CHECK_EQUAL(query.get_tx_key(3), system::null_hash);
}

BC_AUTO_TEST_SUITE_END()
