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
constexpr auto absent_insufficient = system::bit_or(absent,
    system::pool::algorithm::insufficient);
constexpr auto absent_ancestor = system::bit_or(absent,
    system::pool::algorithm::ancestor);

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

BC_AUTO_TEST_CASE(query_pool__set_pool_context__fewer_flags__all_present)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto stored = query.set(tx);
    const auto pooled = query.set_pooled(query.to_tx(tx.hash(false)), tx, context{ bip113 | flags::bip16_rule, 8, 9 }, 0);
    size_t count{ 42 };
    const auto swept = query.set_pool_context(count, context{ bip113, 8, 9 });

    BC_REQUIRE(stored);
    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE(is_zero(count));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, tx)));
}

// get_pool_revalidation

BC_AUTO_TEST_CASE(query_pool__get_pool_revalidation__more_flags__insufficient_listed)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context more{ bip113 | flags::bip16_rule, 8, 9 };
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    size_t count{};
    const auto swept = query.set_pool_context(count, more);
    tx_links links{};
    const auto ec = query.get_pool_revalidation(links, more);

    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(links.size(), one);
    BC_REQUIRE_EQUAL(links.front(), query.to_tx(tx.hash(false)));
}

BC_AUTO_TEST_CASE(query_pool__get_pool_revalidation__lower_height__not_listed)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context lower{ bip113 | flags::bip16_rule, 7, 9 };
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    size_t count{};
    const auto swept = query.set_pool_context(count, lower);
    tx_links links{};
    const auto ec = query.get_pool_revalidation(links, lower);

    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(links.empty());
}

// get_pool_unvalidated

BC_AUTO_TEST_CASE(query_pool__get_pool_unvalidated__more_flags__not_listed)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context more{ bip113 | flags::bip16_rule, 8, 9 };
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    size_t count{};
    const auto swept = query.set_pool_context(count, more);
    tx_links links{};
    const auto ec = query.get_pool_unvalidated(links);

    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(links.empty());
}

// set_pool_unvalidated

BC_AUTO_TEST_CASE(query_pool__set_pool_unvalidated__coinbase_spender__unvalidated_and_listed)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context ctx{ bip113, 8, 9 };
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    size_t marked{};
    const auto unvalidated = query.set_pool_unvalidated(marked, query.to_header(test::genesis.hash()));
    size_t count{};
    const auto swept = query.set_pool_context(count, ctx);
    tx_links links{};
    const auto ec = query.get_pool_unvalidated(links);

    BC_REQUIRE(pooled);
    BC_REQUIRE(unvalidated);
    BC_REQUIRE_EQUAL(marked, 1u);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), absent_insufficient);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), absent_ancestor);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(links.size(), one);
    BC_REQUIRE_EQUAL(links.front(), query.to_tx(parent.hash(false)));
}

BC_AUTO_TEST_CASE(query_pool__set_pool_unvalidated__coinbase_spender__flags_and_median_kept)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto stored = query.set(tx);
    const auto pooled = query.set_pooled(query.to_tx(tx.hash(false)), tx, context{ bip113, 8, 9 }, 0);
    size_t marked{};
    const auto unvalidated = query.set_pool_unvalidated(marked, query.to_header(test::genesis.hash()));
    const auto row = store.pool.first(query.to_tx(tx.hash(false)));
    table::pool_scalar flags{};
    table::pool_scalar height{};
    table::pool_scalar median{};
    const auto read = store.pool.flags.get(row, flags) && store.pool.heights.get(row, height) && store.pool.medians.get(row, median);

    BC_REQUIRE(stored);
    BC_REQUIRE(pooled);
    BC_REQUIRE(unvalidated);
    BC_REQUIRE_EQUAL(marked, 1u);
    BC_REQUIRE(read);
    BC_REQUIRE_EQUAL(flags.value, bip113);
    BC_REQUIRE_EQUAL(height.value, max_uint32);
    BC_REQUIRE_EQUAL(median.value, 9u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_unvalidated__revalidated__present_after_sweep)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context ctx{ bip113, 8, 9 };
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    size_t marked{};
    const auto unvalidated = query.set_pool_unvalidated(marked, query.to_header(test::genesis.hash()));
    size_t excluded{};
    const auto swept = query.set_pool_context(excluded, ctx);
    const auto set = query.set_pooled_context(query.to_tx(tx.hash(false)), ctx, 0);
    size_t restored{ 42 };
    const auto resumed = query.set_pool_context(restored, ctx);

    BC_REQUIRE(pooled);
    BC_REQUIRE(unvalidated);
    BC_REQUIRE_EQUAL(marked, 1u);
    BC_REQUIRE(swept);
    BC_REQUIRE_EQUAL(excluded, 1u);
    BC_REQUIRE(set);
    BC_REQUIRE(resumed);
    BC_REQUIRE(is_zero(restored));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, tx)));
}

BC_AUTO_TEST_CASE(query_pool__set_pool_unvalidated__no_pooled_spender__true_none)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    size_t marked{ 42 };
    BC_REQUIRE(query.set_pool_unvalidated(marked, query.to_header(test::genesis.hash())));
    BC_REQUIRE(is_zero(marked));
}

// set_pooled_context

BC_AUTO_TEST_CASE(query_pool__set_pooled_context__revalidated__present_after_sweep)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context more{ bip113 | flags::bip16_rule, 8, 9 };
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    size_t excluded{};
    size_t restored{ 42 };
    const auto swept = query.set_pool_context(excluded, more);
    const auto set = query.set_pooled_context(query.to_tx(parent.hash(false)), more, 0) && query.set_pooled_context(query.to_tx(child.hash(false)), more, 0);
    const auto resumed = query.set_pool_context(restored, more);

    BC_REQUIRE(pooled);
    BC_REQUIRE(swept);
    BC_REQUIRE(set);
    BC_REQUIRE(resumed);
    BC_REQUIRE_EQUAL(excluded, 2u);
    BC_REQUIRE(is_zero(restored));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, parent)));
    BC_REQUIRE(system::pool::algorithm::is_present(pool_cell(query, store, child)));
}

BC_AUTO_TEST_CASE(query_pool__set_pooled_context__sigops__stored_with_fee_and_size)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const context more{ bip113 | flags::bip16_rule, 8, 9 };
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    const auto link = query.to_tx(tx.hash(false));
    system::pool::algorithm::totals_t before{};
    system::pool::algorithm::totals_t after{};
    const auto first = query.get_pooled_totals(before, link);
    const auto set = query.set_pooled_context(link, more, 42);
    const auto second = query.get_pooled_totals(after, link);

    BC_REQUIRE(pooled);
    BC_REQUIRE(first);
    BC_REQUIRE(set);
    BC_REQUIRE(second);
    BC_REQUIRE_EQUAL(after.fee, before.fee);
    BC_REQUIRE_EQUAL(after.size, before.size);
    BC_REQUIRE_EQUAL(after.sigops, 42u);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled_context__not_pooled__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    BC_REQUIRE(!query.set_pooled_context(42, context{ bip113, 8, 9 }, 0));
}

BC_AUTO_TEST_SUITE_END()
