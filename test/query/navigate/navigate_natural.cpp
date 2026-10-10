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

BOOST_FIXTURE_TEST_SUITE(query_navigate_tests, test::directory_setup_fixture)

// to_candidate

BOOST_AUTO_TEST_CASE(query_navigate__to_candidate__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));

    // initialize pushes the genesis candidate. 
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);

    // key-link translate of actual candidates.
    BOOST_REQUIRE(query.push_candidate(1));
    BOOST_REQUIRE(query.push_candidate(2));
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), 2u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
}

// to_confirmed

BOOST_AUTO_TEST_CASE(query_navigate__to_confirmed__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));

    // initialize pushes the genesis confirmed. 
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);

    // key-link translate of actual confirmeds.
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), 2u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
}

// to_header

BOOST_AUTO_TEST_CASE(query_navigate__to_header__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    header_link link{};

    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), header_link::terminal);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), 0u);
    BOOST_REQUIRE_EQUAL(query.to_header(test::block1.hash()), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.set_code(link, test::block1.header(), test::context, {}, false), error::success);
    BOOST_REQUIRE_EQUAL(link, 1u);
    BOOST_REQUIRE_EQUAL(query.to_header(test::block1.hash()), 1u);
}

// to_tx

BOOST_AUTO_TEST_CASE(query_navigate__to_witness_tx__unsegregated__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));

    // An unsegregated tx has the same hash with and without witness.
    BOOST_REQUIRE_EQUAL(query.to_witness_tx(test::genesis.transactions_ptr()->front()->hash(true)), 0u);
    BOOST_REQUIRE_EQUAL(query.to_witness_tx(test::block1.transactions_ptr()->front()->hash(true)), 1u);
    BOOST_REQUIRE(query.to_witness_tx(system::one_hash).is_terminal());
}

BOOST_AUTO_TEST_CASE(query_navigate__to_witness_tx__segregated_pooled__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    using namespace system::chain;
    const point prevout{ test::genesis.transactions_ptr()->front()->hash(false), 0 };
    const input in{ prevout, script{}, witness{ system::data_stack{ system::data_chunk{ 0x42 } } }, max_uint32 };
    const transaction tx{ 2u, inputs{ in }, outputs{ output{ 1u, script{} } }, 0u };
    tx.inputs_ptr()->front()->prevout = system::to_shared<output>(2u, script{});
    BOOST_REQUIRE(tx.is_segregated());
    BOOST_REQUIRE(tx.hash(true) != tx.hash(false));
    BOOST_REQUIRE(query.set(tx));

    const auto link = query.to_tx(tx.hash(false));
    BOOST_REQUIRE(query.to_witness_tx(tx.hash(true)).is_terminal());
    BOOST_REQUIRE(query.set_pooled(link, tx, database::context{ 0, 1, 0 }));
    BOOST_REQUIRE(query.is_pooled(link));
    BOOST_REQUIRE_EQUAL(query.to_witness_tx(tx.hash(true)), link);
    BOOST_REQUIRE(query.to_witness_tx(tx.hash(false)).is_terminal());
    BOOST_REQUIRE_EQUAL(query.wtxid_records(), query.pool_records());
}

BOOST_AUTO_TEST_CASE(query_navigate__to_tx__transactions__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, {}, false, false));

    // All four blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_tx(test::genesis.transactions_ptr()->front()->hash(true)), 0u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block1.transactions_ptr()->front()->hash(true)), 1u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block2.transactions_ptr()->front()->hash(true)), 2u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block3.transactions_ptr()->front()->hash(true)), tx_link::terminal);
}

// to_filter
// to_output

BOOST_AUTO_TEST_SUITE_END()
