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
#include "../../mocks/pooling.hpp"

BC_FIXTURE_TEST_SUITE(query_pool_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
using test::bip113;
using test::to_spend;
using test::genesis_coinbase;

// get_pooled

BC_AUTO_TEST_CASE(query_pool__get_pooled__no_record__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled.fee, 0u);
}

BC_AUTO_TEST_CASE(query_pool__get_pooled__sufficient__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    const auto& in = *tx.inputs_ptr()->front();
    in.prevout = system::to_shared<output>(0x30, script{});
    in.metadata.parent_tx = 42;
    in.metadata.coinbase = true;
    BC_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::success);
    BC_REQUIRE_EQUAL(pooled.fee, 0x20u);
    BC_REQUIRE_EQUAL(pooled.sigops, tx.signature_operations(false, false));
    BC_REQUIRE_EQUAL(pooled.prevouts.front().parent, 42u);
    BC_REQUIRE(pooled.prevouts.front().coinbase);

    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 9, 10 }), error::success);
}

BC_AUTO_TEST_CASE(query_pool__get_pooled__insufficient__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    const auto& in = *tx.inputs_ptr()->front();
    in.metadata.parent_tx = 42;
    in.metadata.coinbase = false;
    BC_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 7, 9 }), error::unvalidated);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 8 }), error::unvalidated);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113 | flags::bip68_rule, 8, 9 }), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled.fee, 0u);
}

BC_AUTO_TEST_CASE(query_pool__get_pooled__without_bip113__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = 42;
    BC_REQUIRE(query.set_pooled(1, tx, context{ 0, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ 0, 8, 9 }), error::unvalidated);
}

// get_pooled_time

BC_AUTO_TEST_CASE(query_pool__get_pooled_time__pooled__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto tx = to_spend(genesis_coinbase(), 1u);
    BC_REQUIRE(query.set(tx));

    const auto link = query.to_tx(tx.hash(false));
    BC_REQUIRE(query.set_pooled(link, tx, context{ bip113, 8, 9 }, 0x01020304));

    uint32_t time{};
    BC_REQUIRE(query.get_pooled_time(time, link));
    BC_REQUIRE_EQUAL(time, 0x01020304u);
}

BC_AUTO_TEST_CASE(query_pool__get_pooled_time__not_pooled__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    uint32_t time{};
    BC_REQUIRE(!query.get_pooled_time(time, 0));
}

// to_pool_row

BC_AUTO_TEST_CASE(query_pool__to_pool_row__not_pooled__terminal)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    BC_REQUIRE(query.to_pool_row(1).is_terminal());
}

BC_AUTO_TEST_CASE(query_pool__to_pool_row__pooled__rows_in_order)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto pooled = test::pool_populated(query, parent) && test::pool_populated(query, child);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(parent.hash(false))).value, 0u);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(child.hash(false))).value, 1u);
}

// get_pooled_totals

BC_AUTO_TEST_CASE(query_pool__get_pooled_totals__not_pooled__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    system::pool::algorithm::totals_t out{};
    BC_REQUIRE(!query.get_pooled_totals(out, 1));
}

BC_AUTO_TEST_CASE(query_pool__get_pooled_totals__pooled__fee_size_and_sigops)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = test::pool_populated(query, tx);
    system::pool::algorithm::totals_t out{};
    const auto found = query.get_pooled_totals(out, query.to_tx(tx.hash(false)));

    BC_REQUIRE(pooled);
    BC_REQUIRE(found);
    BC_REQUIRE_EQUAL(out.fee, 1'000u);
    BC_REQUIRE_EQUAL(out.weight, tx.weight());
}

BC_AUTO_TEST_SUITE_END()
