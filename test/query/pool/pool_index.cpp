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
using test::archive_confirm;
using test::pool_populated;
using test::pool_cell;

// Absent cells, by cause.
constexpr auto absent = system::pool::algorithm::absent;
constexpr auto absent_rivaled = system::bit_or(absent, system::pool::algorithm::rivaled);
constexpr auto absent_ancestor = system::bit_or(absent, system::pool::algorithm::ancestor);
constexpr auto absent_conflicted = system::bit_or(absent, system::pool::algorithm::conflicted);

// pool index
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_pool__set_pooled__conflict_and_child__one_component)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();

    // Inputs: 0 parent, 1 child, 2 rival.
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'998'000);
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    const auto parent_before = pool_cell(query, store, parent);
    const auto child_before = pool_cell(query, store, child);
    const auto rivaled = pool_populated(query, rival);
    const auto parent_row = store.pool.first(query.to_tx(parent.hash(false)));
    const auto head = store.pool_children.at(parent_row.value);
    const auto ring0 = store.spends_rivals.at(0u);
    const auto ring1 = store.spends_rivals.at(1u);
    const auto ring2 = store.spends_rivals.at(2u);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(parent_before, system::pool::algorithm::unlabeled);
    BC_REQUIRE_EQUAL(child_before, system::pool::algorithm::unlabeled);
    BC_REQUIRE(rivaled);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), 0u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), 0u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, rival), 0u);
    BC_REQUIRE_EQUAL(head, 1u);
    BC_REQUIRE_EQUAL(ring0, 2u);
    BC_REQUIRE_EQUAL(ring1, system::pool::algorithm::none);
    BC_REQUIRE_EQUAL(ring2, 0u);
}

BC_AUTO_TEST_CASE(query_pool__push_pop_confirmed__rival__excluded_and_revived)
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
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto pooled_parent = pool_populated(query, parent);
    const auto pooled_child = pool_populated(query, child);
    const auto pooled_rival = pool_populated(query, rival);
    const auto block = archive_confirm(query, rival);
    const auto rival_confirmed = pool_cell(query, store, rival);
    const auto parent_confirmed = pool_cell(query, store, parent);
    const auto child_confirmed = pool_cell(query, store, child);
    const auto popped = query.pop_confirmed();
    const auto reverted = query.set_pool_block(block, false);

    BC_REQUIRE(pooled_parent);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE(pooled_rival);
    BC_REQUIRE(!block.is_terminal());
    BC_REQUIRE_EQUAL(rival_confirmed, system::pool::algorithm::boundary);
    BC_REQUIRE_EQUAL(parent_confirmed, system::bit_or(absent_rivaled, system::pool::algorithm::conflicted));
    BC_REQUIRE_EQUAL(child_confirmed, absent_ancestor);
    BC_REQUIRE(popped);
    BC_REQUIRE(reverted);
    BC_REQUIRE_EQUAL(pool_cell(query, store, rival), 0u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), 0u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), 0u);
}

BC_AUTO_TEST_CASE(query_pool__push_confirmed__rival__index_unchanged)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto pooled = pool_populated(query, rival);
    const auto before = pool_cell(query, store, rival);
    const auto stored = query.set(test::block1, context{ 0, 1, 0 }, {}, false, true);
    const auto link = query.to_header(test::block1.hash());
    const auto pushed = query.push_candidate(link) && query.push_confirmed(link, true);

    BC_REQUIRE(pooled);
    BC_REQUIRE(stored);
    BC_REQUIRE(pushed);
    BC_REQUIRE_EQUAL(pool_cell(query, store, rival), before);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_block__unpooled_rival_no_prevout_cache__excluded_and_revived)
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
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto pooled_parent = pool_populated(query, parent);
    const auto pooled_child = pool_populated(query, child);
    const auto parent_pooled = pool_cell(query, store, parent);
    const auto child_pooled = pool_cell(query, store, child);
    const auto block = archive_confirm(query, rival);
    const auto parent_confirmed = pool_cell(query, store, parent);
    const auto child_confirmed = pool_cell(query, store, child);
    const auto popped = query.pop_confirmed();
    const auto reverted = query.set_pool_block(block, false);

    BC_REQUIRE(pooled_parent);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE(!block.is_terminal());
    BC_REQUIRE(!query.is_pooled(query.to_tx(rival.hash(false))));
    BC_REQUIRE_EQUAL(parent_confirmed, absent_conflicted);
    BC_REQUIRE_EQUAL(child_confirmed, absent_ancestor);
    BC_REQUIRE(popped);
    BC_REQUIRE(reverted);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), parent_pooled);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), child_pooled);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__reorganized_parent__children_adopted)
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
    const auto confirmed = !archive_confirm(query, parent).is_terminal();
    const auto pooled = pool_populated(query, child);
    const auto before = store.spends_rows.at(0u);
    const auto popped = query.pop_confirmed();
    const auto link = query.to_tx(parent.hash(false));
    const auto repooled = query.set_pooled(link, parent, context{ bip113, 8, 9 }, 0);
    const auto after = store.spends_rows.at(0u);
    const auto head = store.pool_children.at(1u);
    const auto rivaled = pool_populated(query, to_spend(genesis_coinbase(), 4'999'998'500));

    BC_REQUIRE(confirmed);
    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(before, system::pool::algorithm::none);
    BC_REQUIRE(popped);
    BC_REQUIRE(repooled);
    BC_REQUIRE_EQUAL(after, 1u);
    BC_REQUIRE_EQUAL(head, 0u);
    BC_REQUIRE(rivaled);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), 0u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), 0u);
}

BC_AUTO_TEST_SUITE_END()
