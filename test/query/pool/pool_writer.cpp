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

// set_pooled

BC_AUTO_TEST_CASE(query_pool__set_pooled__disabled__no_record)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = 42;
    BC_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
    BC_REQUIRE_EQUAL(query.pool_body_size(), zero);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__time__stored)
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
    BC_REQUIRE(query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0x01020304));

    table::pool::record record{};
    BC_REQUIRE(store.pool.get(store.pool.first(tx_link{ 1 }), record));
    BC_REQUIRE_EQUAL(record.time, 0x01020304u);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__given_sigops__stored)
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
    size_t repooled{ 42 };
    BC_REQUIRE(query.set_pooled(repooled, 1, tx, context{ bip113, 8, 9 }, 0, 7, false));
    BC_REQUIRE_EQUAL(repooled, 0u);

    test::pool_totals totals{};
    BC_REQUIRE(query.get_pooled_totals(totals, 1));
    BC_REQUIRE_EQUAL(totals.sigops, 7u);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__unlinked_parent__resolved)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    tx_link parent{};
    BC_REQUIRE_EQUAL(query.set_code(parent, test::tx4), error::success);

    const auto& tx = test::tx_spend_tx4;
    tx.inputs_ptr()->front()->metadata.parent_tx = max_uint32;
    BC_REQUIRE(query.set_pooled(2, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 2, context{ bip113, 8, 9 }), error::success);
    BC_REQUIRE_EQUAL(pooled.prevouts.front().parent, parent);
    BC_REQUIRE(!pooled.prevouts.front().coinbase);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__missing_parent__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto& tx = test::tx_spend_one_hash;
    tx.inputs_ptr()->front()->metadata.parent_tx = max_uint32;
    BC_REQUIRE(!query.set_pooled(1, tx, context{ bip113, 8, 9 }, 0));

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query.get_pooled(pooled, 1, context{ bip113, 8, 9 }), error::unvalidated);
}

BC_AUTO_TEST_SUITE_END()
