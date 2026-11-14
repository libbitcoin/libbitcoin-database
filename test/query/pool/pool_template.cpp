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
using test::block_limit;
using test::pool_projection;
using test::to_spend;
using test::genesis_coinbase;
using test::pool_populated;

// get_pool_template
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_pool__get_pool_template__empty__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    tx_links out{ 42 };
    pool_projection projection{};
    const auto ec = query.get_pool_template(out, projection, block_limit, 25, 20, 1'024);

    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(out.empty());
}

BC_AUTO_TEST_CASE(query_pool__get_pool_template__chain_outbids_rival__chain_ascending)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();

    // The parent and child pay 2,000 together, and the rival of the parent 1,500.
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto pooled_parent = pool_populated(query, parent);
    const auto pooled_child = pool_populated(query, child);
    const auto pooled_rival = pool_populated(query, rival);
    tx_links out{};
    pool_projection projection{};
    const auto ec = query.get_pool_template(out, projection, block_limit, 25, 20, 1'024);

    BC_REQUIRE(pooled_parent);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE(pooled_rival);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 2u);
    BC_REQUIRE_EQUAL(out[0], query.to_tx(parent.hash(false)));
    BC_REQUIRE_EQUAL(out[1], query.to_tx(child.hash(false)));
}

BC_AUTO_TEST_CASE(query_pool__get_pool_template__rival_outbids_chain__rival_only)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();

    // The parent and child pay 2,000 together, and the rival of the parent 2,500.
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto rival = to_spend(genesis_coinbase(), 4'999'997'500);
    const auto pooled_parent = pool_populated(query, parent);
    const auto pooled_child = pool_populated(query, child);
    const auto pooled_rival = pool_populated(query, rival);
    tx_links out{};
    pool_projection projection{};
    const auto ec = query.get_pool_template(out, projection, block_limit, 25, 20, 1'024);

    BC_REQUIRE(pooled_parent);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE(pooled_rival);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 1u);
    BC_REQUIRE_EQUAL(out[0], query.to_tx(rival.hash(false)));
}

BC_AUTO_TEST_CASE(query_pool__get_pool_template__chain_over_budget__leading_tx)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    tx_links out{};
    pool_projection projection{};
    const auto ec = query.get_pool_template(out, projection, { 0, size, 80'000 }, 25, 20, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 1u);
    BC_REQUIRE_EQUAL(out[0], query.to_tx(parent.hash(false)));
}

BC_AUTO_TEST_CASE(query_pool__get_pool_template__chain_over_budget__first_block_projected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    tx_links out{};
    pool_projection projection{};
    const auto ec = query.get_pool_template(out, projection, { 0, size, 80'000 }, 25, 20, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.marks.size(), 2u);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 1u);
    BC_REQUIRE(!projection.exhausted);
}

BC_AUTO_TEST_SUITE_END()
