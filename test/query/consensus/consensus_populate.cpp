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

BC_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

// populate_pooled

BC_AUTO_TEST_CASE(query_consensus__populate_pooled__sufficient__external_metadata)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const transaction source{ test::tx_spend_one_hash.to_data(true), true };
    const auto& source_in = *source.inputs_ptr()->front();
    source_in.prevout = system::to_shared<output>(0x30, script{});
    source_in.metadata.parent_tx = 42;
    source_in.metadata.coinbase = true;
    BC_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }, 0));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BC_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 9, 10 }), error::success);
    BC_REQUIRE_EQUAL(pooled.fee, 0x20u);
    BC_REQUIRE_EQUAL(in.metadata.parent_tx, 42u);
    BC_REQUIRE(in.metadata.coinbase);
    BC_REQUIRE(!in.prevout);
}

BC_AUTO_TEST_CASE(query_consensus__populate_pooled__internal_spend__terminal_metadata)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const transaction source{ test::tx_spend_one_hash.to_data(true), true };
    const auto& source_in = *source.inputs_ptr()->front();
    source_in.prevout = system::to_shared<output>(0x30, script{});
    source_in.metadata.parent_tx = 42;
    source_in.metadata.coinbase = true;
    BC_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }, 0));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.coinbase = false;

    pooled_tx pooled{};
    BC_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 8, 9 }), error::success);
    BC_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
    BC_REQUIRE(!in.metadata.coinbase);
}

BC_AUTO_TEST_CASE(query_consensus__populate_pooled__insufficient__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const transaction source{ test::tx_spend_one_hash.to_data(true), true };
    const auto& source_in = *source.inputs_ptr()->front();
    source_in.prevout = system::to_shared<output>(0x30, script{});
    source_in.metadata.parent_tx = 42;
    BC_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }, 0));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BC_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BC_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
}

// populate_with_metadata
// ----------------------------------------------------------------------------

// METADATA IS SETTABLE ON CONST TEST OBJECTS.
// COPY CONSTRUCTION CREATES SAHRED POINTER REFERENCES.
// SO POPULATE ON CONST OBJECTS HAS SIDE EFFECTS IF THE OBJECTS SPAN TESTS.
const auto& clean_(const auto& block_or_tx) NOEXCEPT
{
    const auto inputs = block_or_tx.inputs_ptr();
    for (const auto& input : *inputs)
    {
        input->prevout.reset();
        input->metadata = system::chain::prevout{};
    }

    return block_or_tx;
}

// First four blocks have only coinbase txs.
BC_AUTO_TEST_CASE(query_consensus__populate_with_metadata__null_prevouts__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1, test::context, {}, false, false));
    BC_CHECK(query.set(test::block2, test::context, {}, false, false));
    BC_CHECK(query.set(test::block3, test::context, {}, false, false));

    const auto& copy = clean_(test::genesis);
    const auto& copy1 = clean_(test::block1);
    const auto& copy2 = clean_(test::block2);
    const auto& copy3 = clean_(test::block3);

    BC_CHECK(query.populate_with_metadata(copy));
    BC_CHECK(query.populate_with_metadata(copy1));
    BC_CHECK(query.populate_with_metadata(copy2));
    BC_CHECK(query.populate_with_metadata(copy3));
}

BC_AUTO_TEST_CASE(query_consensus__populate_with_metadata__partial_prevouts__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BC_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BC_CHECK(query.set(test::tx4));

    const auto& block1a = clean_(test::block1a);

    // Block populate treates first tx as null point.
    BC_CHECK(query.populate_with_metadata(block1a));
    BC_CHECK(!query.populate_with_metadata(*block1a.transactions_ptr()->at(0)));
    BC_CHECK(!query.populate_with_metadata(*block1a.inputs_ptr()->at(0)));
    BC_CHECK(!query.populate_with_metadata(*block1a.inputs_ptr()->at(2)));

    // Block populate treates first tx as null point and other has missing prevouts.
    const auto& block2a = clean_(test::block2a);
    BC_CHECK(!query.populate_with_metadata(block2a));
    BC_CHECK(query.populate_with_metadata(*block2a.transactions_ptr()->at(0)));
    BC_CHECK(!query.populate_with_metadata(*block2a.transactions_ptr()->at(1)));
    BC_CHECK(query.populate_with_metadata(*block2a.inputs_ptr()->at(0)));
    BC_CHECK(!query.populate_with_metadata(*block2a.inputs_ptr()->at(3)));

    const auto& tx4 = clean_(test::tx4);
    BC_CHECK(query.populate_with_metadata(tx4));
    BC_CHECK(query.populate_with_metadata(*tx4.inputs_ptr()->at(0)));
    BC_CHECK(query.populate_with_metadata(*tx4.inputs_ptr()->at(1)));
}

BC_AUTO_TEST_CASE(query_consensus__populate_with_metadata__metadata__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BC_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BC_CHECK(query.set(test::tx4));
    BC_CHECK(!query.is_coinbase(1));

    // Genesis only has coinbase, which does not spend.
    const auto& genesis = clean_(test::genesis);

    BC_CHECK(!genesis.inputs_ptr()->at(0)->prevout);
    BC_CHECK(genesis.inputs_ptr()->at(0)->metadata.coinbase);
    BC_CHECK_EQUAL(genesis.inputs_ptr()->at(0)->metadata.parent_tx, max_uint32);

    BC_CHECK(query.populate_with_metadata(genesis));

    BC_CHECK(!genesis.inputs_ptr()->at(0)->prevout);
    BC_CHECK(genesis.inputs_ptr()->at(0)->metadata.coinbase);
    BC_CHECK_EQUAL(genesis.inputs_ptr()->at(0)->metadata.parent_tx, max_uint32);

    // Transaction population.
    const auto& tx4 = clean_(test::tx4);

    BC_CHECK(!tx4.inputs_ptr()->at(0)->prevout);
    BC_CHECK(tx4.inputs_ptr()->at(0)->metadata.coinbase);
    BC_CHECK_EQUAL(tx4.inputs_ptr()->at(0)->metadata.parent_tx, max_uint32);
    BC_CHECK(!tx4.inputs_ptr()->at(1)->prevout);
    BC_CHECK(tx4.inputs_ptr()->at(1)->metadata.coinbase);
    BC_CHECK_EQUAL(tx4.inputs_ptr()->at(1)->metadata.parent_tx, max_uint32);

    BC_CHECK(query.populate_with_metadata(tx4));

    // TODO: test non-coinbase and other parent.
    // spent/mtp are defaults, coinbase/parent are set (to non-default values).
    BC_CHECK(tx4.inputs_ptr()->at(0)->prevout);
    BC_CHECK(!tx4.inputs_ptr()->at(0)->metadata.coinbase);
    BC_CHECK_EQUAL(tx4.inputs_ptr()->at(0)->metadata.parent_tx, 1u);
    BC_CHECK(tx4.inputs_ptr()->at(1)->prevout);
    BC_CHECK(!tx4.inputs_ptr()->at(1)->metadata.coinbase);
    BC_CHECK_EQUAL(tx4.inputs_ptr()->at(1)->metadata.parent_tx, 1u);
}

// A free tx conflicts with any spender, but only a confirmed one has height.
BC_AUTO_TEST_CASE(query_consensus__populate_with_metadata__pool_spender__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));

    // tx5 spends the first output of the first tx of block1a, as do tx4 and
    // the first transaction of block2a.
    const auto& unspent = clean_(test::tx5);
    BC_CHECK(query.populate_with_metadata(unspent, true, true));
    BC_CHECK_EQUAL(unspent.inputs_ptr()->at(0)->metadata.spender_height, max_uint32);

    BC_CHECK(query.set(test::tx4));

    const auto& pooled = clean_(test::tx5);
    BC_CHECK(query.populate_with_metadata(pooled, true, true));
    BC_CHECK_EQUAL(pooled.inputs_ptr()->at(0)->metadata.spender_height, 0u);

    // The spender is unassociated, so it is not a confirmed spender.
    const auto& chained = clean_(test::tx5);
    BC_CHECK(query.populate_with_metadata(chained, true, false));
    BC_CHECK_EQUAL(chained.inputs_ptr()->at(0)->metadata.spender_height, max_uint32);

    BC_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BC_CHECK(query.set_strong(2));

    const auto& confirmed = clean_(test::tx5);
    BC_CHECK(query.populate_with_metadata(confirmed, true, true));
    BC_CHECK_EQUAL(confirmed.inputs_ptr()->at(0)->metadata.spender_height, 2u);

    const auto& strong = clean_(test::tx5);
    BC_CHECK(query.populate_with_metadata(strong, true, false));
    BC_CHECK_EQUAL(strong.inputs_ptr()->at(0)->metadata.spender_height, 2u);
}

// populate_without_metadata
// ----------------------------------------------------------------------------

// First four blocks have only coinbase txs.
BC_AUTO_TEST_CASE(query_consensus__populate_without_metadata__null_prevouts__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1, test::context, {}, false, false));
    BC_CHECK(query.set(test::block2, test::context, {}, false, false));
    BC_CHECK(query.set(test::block3, test::context, {}, false, false));
    BC_CHECK(query.populate_without_metadata(test::genesis));
    BC_CHECK(query.populate_without_metadata(test::block1));
    BC_CHECK(query.populate_without_metadata(test::block2));
    BC_CHECK(query.populate_without_metadata(test::block3));
}

BC_AUTO_TEST_CASE(query_consensus__populate_without_metadata__partial_prevouts__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));
    BC_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BC_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BC_CHECK(query.set(test::tx4));

    // Block populate treates first tx as null point.
    BC_CHECK(query.populate_without_metadata(test::block1a));
    BC_CHECK(!query.populate_without_metadata(*test::block1a.transactions_ptr()->at(0)));
    BC_CHECK(!query.populate_without_metadata(*test::block1a.inputs_ptr()->at(0)));
    BC_CHECK(!query.populate_without_metadata(*test::block1a.inputs_ptr()->at(2)));

    // Block populate treates first tx as null point and other has missing prevouts.
    BC_CHECK(!query.populate_without_metadata(test::block2a));
    BC_CHECK(query.populate_without_metadata(*test::block2a.transactions_ptr()->at(0)));
    BC_CHECK(!query.populate_without_metadata(*test::block2a.transactions_ptr()->at(1)));
    BC_CHECK(query.populate_without_metadata(*test::block2a.inputs_ptr()->at(0)));
    BC_CHECK(!query.populate_without_metadata(*test::block2a.inputs_ptr()->at(3)));

    BC_CHECK(query.populate_without_metadata(test::tx4));
    BC_CHECK(query.populate_without_metadata(*test::tx4.inputs_ptr()->at(0)));
    BC_CHECK(query.populate_without_metadata(*test::tx4.inputs_ptr()->at(1)));
}

BC_AUTO_TEST_SUITE_END()
