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
using test::pool_populated;
using test::pool_cell;

// Absent cells, by cause.
constexpr auto absent = system::pool::algorithm::absent;
constexpr auto absent_insufficient = system::bit_or(absent, system::pool::algorithm::insufficient);
constexpr auto absent_ancestor = system::bit_or(absent, system::pool::algorithm::ancestor);

// set_pool_context

BC_AUTO_TEST_CASE(query_pool__set_pool_context__empty__true_none)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    size_t count{ 42 };
    BC_REQUIRE(query.set_pool_context(count, context{ bip113, 8, 9 }));
    BC_REQUIRE(is_zero(count));
}

BC_AUTO_TEST_CASE(query_pool__set_pool_context__same_context__all_present)
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
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    size_t count{ 42 };
    const auto swept = query.set_pool_context(count, context{ bip113, 8, 9 });

    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE(is_zero(count));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, parent)));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, child)));
}

BC_AUTO_TEST_CASE(query_pool__set_pool_context__lower_height__later_row_and_child_absent)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto other = to_spend(genesis_coinbase(), 4'999'997'000);
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto pooled_other = pool_populated(query, other, 7);
    const auto pooled_parent = pool_populated(query, parent, 9);
    const auto pooled_child = pool_populated(query, child, 8);
    size_t count{};
    const auto swept = query.set_pool_context(count, context{ bip113, 8, 9 });

    BC_REQUIRE(pooled_other);
    BC_REQUIRE(pooled_parent);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, other)));
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), absent_insufficient);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), absent_ancestor);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_context__height_regained__all_present)
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
    const auto pooled_parent = pool_populated(query, parent, 9);
    const auto pooled_child = pool_populated(query, child, 8);
    size_t lowered{};
    size_t regained{ 42 };
    const auto swept = query.set_pool_context(lowered, context{ bip113, 8, 9 });
    const auto restored = query.set_pool_context(regained, context{ bip113, 9, 9 });

    BC_REQUIRE(pooled_parent);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE(swept);
    BC_REQUIRE(restored);
    BC_REQUIRE_EQUAL(lowered, 1u);
    BC_REQUIRE(is_zero(regained));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, parent)));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, child)));
}

BC_AUTO_TEST_CASE(query_pool__set_pool_context__other_flags__all_insufficient)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, parent);
    size_t count{};
    const auto swept = query.set_pool_context(count, context{ flags::bip16_rule, 8, 9 });

    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), absent_insufficient);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_context__same_flags_without_bip113__insufficient)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context ctx{ flags::bip16_rule, 8, 9 };
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto stored = query.set(tx);
    const auto pooled = query.set_pooled(query.to_tx(tx.hash(false)), tx, ctx, 0);
    size_t count{};
    const auto swept = query.set_pool_context(count, ctx);

    BC_REQUIRE(stored);
    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, tx), absent_insufficient);
}

BC_AUTO_TEST_SUITE_END()
