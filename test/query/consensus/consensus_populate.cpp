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

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

// populate_pooled

BOOST_AUTO_TEST_CASE(query_consensus__populate_pooled__sufficient__external_metadata)
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
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }, 0));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 9, 10 }), error::success);
    BOOST_REQUIRE_EQUAL(pooled.fee, 0x20u);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, 42u);
    BOOST_REQUIRE(in.metadata.coinbase);
    BOOST_REQUIRE(!in.prevout);
}

BOOST_AUTO_TEST_CASE(query_consensus__populate_pooled__internal_spend__terminal_metadata)
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
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }, 0));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.coinbase = false;

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 8, 9 }), error::success);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
    BOOST_REQUIRE(!in.metadata.coinbase);
}

BOOST_AUTO_TEST_CASE(query_consensus__populate_pooled__insufficient__unvalidated)
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
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }, 0));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
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
BOOST_AUTO_TEST_CASE(query_consensus__populate_with_metadata__null_prevouts__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block3, test::context, {}, false, false));

    const auto& copy = clean_(test::genesis);
    const auto& copy1 = clean_(test::block1);
    const auto& copy2 = clean_(test::block2);
    const auto& copy3 = clean_(test::block3);

    BOOST_CHECK(query.populate_with_metadata(copy));
    BOOST_CHECK(query.populate_with_metadata(copy1));
    BOOST_CHECK(query.populate_with_metadata(copy2));
    BOOST_CHECK(query.populate_with_metadata(copy3));
}

BOOST_AUTO_TEST_CASE(query_consensus__populate_with_metadata__partial_prevouts__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::tx4));

    const auto& block1a = clean_(test::block1a);

    // Block populate treates first tx as null point.
    BOOST_CHECK(query.populate_with_metadata(block1a));
    BOOST_CHECK(!query.populate_with_metadata(*block1a.transactions_ptr()->at(0)));
    BOOST_CHECK(!query.populate_with_metadata(*block1a.inputs_ptr()->at(0)));
    BOOST_CHECK(!query.populate_with_metadata(*block1a.inputs_ptr()->at(2)));

    // Block populate treates first tx as null point and other has missing prevouts.
    const auto& block2a = clean_(test::block2a);
    BOOST_CHECK(!query.populate_with_metadata(block2a));
    BOOST_CHECK(query.populate_with_metadata(*block2a.transactions_ptr()->at(0)));
    BOOST_CHECK(!query.populate_with_metadata(*block2a.transactions_ptr()->at(1)));
    BOOST_CHECK(query.populate_with_metadata(*block2a.inputs_ptr()->at(0)));
    BOOST_CHECK(!query.populate_with_metadata(*block2a.inputs_ptr()->at(3)));

    const auto& tx4 = clean_(test::tx4);
    BOOST_CHECK(query.populate_with_metadata(tx4));
    BOOST_CHECK(query.populate_with_metadata(*tx4.inputs_ptr()->at(0)));
    BOOST_CHECK(query.populate_with_metadata(*tx4.inputs_ptr()->at(1)));
}

BOOST_AUTO_TEST_CASE(query_consensus__populate_with_metadata__metadata__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::tx4));
    BOOST_CHECK(!query.is_coinbase(1));

    // Genesis only has coinbase, which does not spend.
    const auto& genesis = clean_(test::genesis);

    BOOST_CHECK(!genesis.inputs_ptr()->at(0)->prevout);
    BOOST_CHECK(genesis.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_CHECK_EQUAL(genesis.inputs_ptr()->at(0)->metadata.parent_tx, max_uint32);

    BOOST_CHECK(query.populate_with_metadata(genesis));

    BOOST_CHECK(!genesis.inputs_ptr()->at(0)->prevout);
    BOOST_CHECK(genesis.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_CHECK_EQUAL(genesis.inputs_ptr()->at(0)->metadata.parent_tx, max_uint32);

    // Transaction population.
    const auto& tx4 = clean_(test::tx4);

    BOOST_CHECK(!tx4.inputs_ptr()->at(0)->prevout);
    BOOST_CHECK(tx4.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_CHECK_EQUAL(tx4.inputs_ptr()->at(0)->metadata.parent_tx, max_uint32);
    BOOST_CHECK(!tx4.inputs_ptr()->at(1)->prevout);
    BOOST_CHECK(tx4.inputs_ptr()->at(1)->metadata.coinbase);
    BOOST_CHECK_EQUAL(tx4.inputs_ptr()->at(1)->metadata.parent_tx, max_uint32);

    BOOST_CHECK(query.populate_with_metadata(tx4));

    // TODO: test non-coinbase and other parent.
    // spent/mtp are defaults, coinbase/parent are set (to non-default values).
    BOOST_CHECK(tx4.inputs_ptr()->at(0)->prevout);
    BOOST_CHECK(!tx4.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_CHECK_EQUAL(tx4.inputs_ptr()->at(0)->metadata.parent_tx, 1u);
    BOOST_CHECK(tx4.inputs_ptr()->at(1)->prevout);
    BOOST_CHECK(!tx4.inputs_ptr()->at(1)->metadata.coinbase);
    BOOST_CHECK_EQUAL(tx4.inputs_ptr()->at(1)->metadata.parent_tx, 1u);
}

// A free tx conflicts with any spender, but only a confirmed one has height.
BOOST_AUTO_TEST_CASE(query_consensus__populate_with_metadata__pool_spender__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));

    // tx5 spends the first output of the first tx of block1a, as do tx4 and
    // the first transaction of block2a.
    const auto& unspent = clean_(test::tx5);
    BOOST_CHECK(query.populate_with_metadata(unspent, true, true));
    BOOST_CHECK_EQUAL(unspent.inputs_ptr()->at(0)->metadata.spender_height, max_uint32);

    BOOST_CHECK(query.set(test::tx4));

    const auto& pooled = clean_(test::tx5);
    BOOST_CHECK(query.populate_with_metadata(pooled, true, true));
    BOOST_CHECK_EQUAL(pooled.inputs_ptr()->at(0)->metadata.spender_height, 0u);

    // The spender is unassociated, so it is not a confirmed spender.
    const auto& chained = clean_(test::tx5);
    BOOST_CHECK(query.populate_with_metadata(chained, true, false));
    BOOST_CHECK_EQUAL(chained.inputs_ptr()->at(0)->metadata.spender_height, max_uint32);

    BOOST_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_CHECK(query.set_strong(2));

    const auto& confirmed = clean_(test::tx5);
    BOOST_CHECK(query.populate_with_metadata(confirmed, true, true));
    BOOST_CHECK_EQUAL(confirmed.inputs_ptr()->at(0)->metadata.spender_height, 2u);

    const auto& strong = clean_(test::tx5);
    BOOST_CHECK(query.populate_with_metadata(strong, true, false));
    BOOST_CHECK_EQUAL(strong.inputs_ptr()->at(0)->metadata.spender_height, 2u);
}

// populate_without_metadata
// ----------------------------------------------------------------------------

// First four blocks have only coinbase txs.
BOOST_AUTO_TEST_CASE(query_consensus__populate_without_metadata__null_prevouts__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block3, test::context, {}, false, false));
    BOOST_CHECK(query.populate_without_metadata(test::genesis));
    BOOST_CHECK(query.populate_without_metadata(test::block1));
    BOOST_CHECK(query.populate_without_metadata(test::block2));
    BOOST_CHECK(query.populate_without_metadata(test::block3));
}

BOOST_AUTO_TEST_CASE(query_consensus__populate_without_metadata__partial_prevouts__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::tx4));

    // Block populate treates first tx as null point.
    BOOST_CHECK(query.populate_without_metadata(test::block1a));
    BOOST_CHECK(!query.populate_without_metadata(*test::block1a.transactions_ptr()->at(0)));
    BOOST_CHECK(!query.populate_without_metadata(*test::block1a.inputs_ptr()->at(0)));
    BOOST_CHECK(!query.populate_without_metadata(*test::block1a.inputs_ptr()->at(2)));

    // Block populate treates first tx as null point and other has missing prevouts.
    BOOST_CHECK(!query.populate_without_metadata(test::block2a));
    BOOST_CHECK(query.populate_without_metadata(*test::block2a.transactions_ptr()->at(0)));
    BOOST_CHECK(!query.populate_without_metadata(*test::block2a.transactions_ptr()->at(1)));
    BOOST_CHECK(query.populate_without_metadata(*test::block2a.inputs_ptr()->at(0)));
    BOOST_CHECK(!query.populate_without_metadata(*test::block2a.inputs_ptr()->at(3)));

    BOOST_CHECK(query.populate_without_metadata(test::tx4));
    BOOST_CHECK(query.populate_without_metadata(*test::tx4.inputs_ptr()->at(0)));
    BOOST_CHECK(query.populate_without_metadata(*test::tx4.inputs_ptr()->at(1)));
}

BOOST_AUTO_TEST_SUITE_END()
