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
    BOOST_REQUIRE(query.set_pooled(1, source, context{ bip113, 8, 9 }));

    const transaction tx{ test::tx_spend_one_hash.to_data(true), true };
    const auto& in = *tx.inputs_ptr()->front();

    pooled_tx pooled{};
    BOOST_REQUIRE_EQUAL(query.populate_pooled(pooled, tx, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BOOST_REQUIRE_EQUAL(in.metadata.parent_tx, max_uint32);
}

BOOST_AUTO_TEST_SUITE_END()
