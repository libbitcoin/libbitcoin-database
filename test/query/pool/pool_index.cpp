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
using test::block_limit;
using test::pool_projection;

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
    const auto head = store.pool_index.children.at(parent_row.value);
    const auto ring0 = store.spends_index.rivals.at(0u);
    const auto ring1 = store.spends_index.rivals.at(1u);
    const auto ring2 = store.spends_index.rivals.at(2u);

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

// The rival is pooled after the block's prevouts and conflicts are cached.
BC_AUTO_TEST_CASE(query_pool__set_pool_block__rival_pooled_after_prevout_cache__excluded)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto spender = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto rival = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto block = test::to_block(spender);
    const auto stored = query.set(block, context{ 0, 1, 0 }, {}, false, true);
    const auto link = query.to_header(block.hash());
    const auto cached = query.populate_with_metadata(block) && query.set_prevouts(link, block);
    const auto pooled = pool_populated(query, rival);
    const auto before = pool_cell(query, store, rival);
    const auto pushed = query.push_candidate(link) && query.push_confirmed(link, true);
    const auto applied = query.set_pool_block(link, true);

    BC_REQUIRE(stored);
    BC_REQUIRE(cached);
    BC_REQUIRE(pooled);
    BC_REQUIRE(system::pool::algorithm::is_present(before));
    BC_REQUIRE(pushed);
    BC_REQUIRE(applied);
    BC_REQUIRE_EQUAL(pool_cell(query, store, rival), absent_conflicted);
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

BC_AUTO_TEST_CASE(query_pool__set_pooled__reorganized_parent__child_repooled_after)
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
    const auto popped = query.pop_confirmed();
    const auto link = query.to_tx(parent.hash(false));
    size_t count{};
    const auto repooled = query.set_pooled(count, link, parent, context{ bip113, 8, 9 }, 0);
    const auto rivaled = pool_populated(query, to_spend(genesis_coinbase(), 4'999'998'500));
    constexpr auto purged = system::bit_or(system::pool::algorithm::absent, system::pool::algorithm::purged);

    BC_REQUIRE(confirmed);
    BC_REQUIRE(pooled);
    BC_REQUIRE(popped);
    BC_REQUIRE(repooled);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE(rivaled);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(child.hash(false))).value, 2u);
    BC_REQUIRE_EQUAL(store.pool_index.cells.at(0u), purged);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(0u), system::pool::algorithm::none);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(2u), 1u);
    BC_REQUIRE_EQUAL(store.pool_index.children.at(1u), 2u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), 1u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), 1u);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__reorganized_parent__descendants_repooled_parents_first)
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
    const auto grandchild = to_spend(child.hash(false), 4'999'997'000);
    const auto confirmed = !archive_confirm(query, parent).is_terminal();
    const auto pooled = pool_populated(query, child) && pool_populated(query, grandchild);
    const auto popped = query.pop_confirmed();
    const auto link = query.to_tx(parent.hash(false));
    const auto repooled = query.set_pooled(link, parent, context{ bip113, 8, 9 }, 0);
    tx_links listed{};
    const auto listed_ec = query.get_confirmable_txs(listed);
    tx_links out{};
    pool_projection projection{};
    const auto ec = query.get_pool_template(out, projection, block_limit, 25, 0, 20, 1'024);
    const tx_links expected{ link, query.to_tx(child.hash(false)), query.to_tx(grandchild.hash(false)) };

    BC_REQUIRE(confirmed);
    BC_REQUIRE(pooled);
    BC_REQUIRE(popped);
    BC_REQUIRE(repooled);
    BC_REQUIRE_EQUAL(query.to_pool_row(link).value, 2u);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(child.hash(false))).value, 3u);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(grandchild.hash(false))).value, 4u);
    BC_REQUIRE(!listed_ec);
    BC_REQUIRE(listed == expected);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(out == expected);
}

BC_AUTO_TEST_CASE(query_pool__set_pooled__reorganized_parent__descendant_by_two_paths_repooled_after_both)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // Rows: 0 child, 1 grandchild, 2 joined (spends parent and grandchild).
    store.set_pooling();
    const transaction parent{ 1u, inputs{ input{ point{ genesis_coinbase(), 0 }, script{}, max_uint32 } }, outputs{ output{ 2'499'999'000, script{} }, output{ 2'499'999'000, script{} } }, 0u };
    const auto child = to_spend(parent.hash(false), 2'499'998'000);
    const auto grandchild = to_spend(child.hash(false), 2'499'997'000);
    const transaction joined{ 1u, inputs{ input{ point{ parent.hash(false), 1 }, script{}, max_uint32 }, input{ point{ grandchild.hash(false), 0 }, script{}, max_uint32 } }, outputs{ output{ 4'999'995'000, script{} } }, 0u };
    const auto confirmed = !archive_confirm(query, parent).is_terminal();
    const auto pooled = pool_populated(query, child) && pool_populated(query, grandchild) && pool_populated(query, joined);
    const auto popped = query.pop_confirmed();
    const auto link = query.to_tx(parent.hash(false));
    size_t count{};
    const auto repooled = query.set_pooled(count, link, parent, context{ bip113, 8, 9 }, 0);

    BC_REQUIRE(confirmed);
    BC_REQUIRE(pooled);
    BC_REQUIRE(popped);
    BC_REQUIRE(repooled);
    BC_REQUIRE_EQUAL(count, 3u);
    BC_REQUIRE_EQUAL(query.to_pool_row(link).value, 3u);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(child.hash(false))).value, 4u);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(grandchild.hash(false))).value, 5u);
    BC_REQUIRE_EQUAL(query.to_pool_row(query.to_tx(joined.hash(false))).value, 6u);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(5u), 3u);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(6u), 4u);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(7u), 3u);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(8u), 5u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_index__repooled_child__purged_row_kept_purged)
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
    const auto popped = query.pop_confirmed();
    const auto link = query.to_tx(parent.hash(false));
    const auto repooled = query.set_pooled(link, parent, context{ bip113, 8, 9 }, 0);
    const auto rebuilt = query.set_pool_index();
    constexpr auto purged = system::bit_or(system::pool::algorithm::absent, system::pool::algorithm::purged);

    BC_REQUIRE(confirmed);
    BC_REQUIRE(pooled);
    BC_REQUIRE(popped);
    BC_REQUIRE(repooled);
    BC_REQUIRE(rebuilt);
    BC_REQUIRE_EQUAL(store.pool_index.cells.at(0u), purged);
    BC_REQUIRE_EQUAL(store.spends_index.rows.at(2u), 1u);
    BC_REQUIRE_EQUAL(pool_cell(query, store, parent), system::pool::algorithm::unlabeled);
    BC_REQUIRE_EQUAL(pool_cell(query, store, child), system::pool::algorithm::unlabeled);
}

// pool top
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_pool__get_pool_top__initialized__terminal)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.get_pool_top().is_terminal());
}

BC_AUTO_TEST_CASE(query_pool__set_pool_top__link__stored)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set_pool_top(header_link{ 42 }));
    BC_REQUIRE_EQUAL(query.get_pool_top().value, 42u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_block__confirmed_empty_pool__pool_top_block)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto link = archive_confirm(query, to_spend(genesis_coinbase(), 4'999'999'000));

    BC_REQUIRE(!link.is_terminal());
    BC_REQUIRE_EQUAL(query.pool_records(), zero);
    BC_REQUIRE_EQUAL(query.get_pool_top().value, 1u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_block__confirmed__pool_top_block)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto pooled = pool_populated(query, to_spend(genesis_coinbase(), 4'999'998'500));
    const auto link = archive_confirm(query, to_spend(genesis_coinbase(), 4'999'999'000));

    BC_REQUIRE(pooled);
    BC_REQUIRE(!link.is_terminal());
    BC_REQUIRE_EQUAL(query.get_pool_top().value, 1u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_block__reorganized__pool_top_unchanged)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto pooled = pool_populated(query, to_spend(genesis_coinbase(), 4'999'998'500));
    const auto link = archive_confirm(query, to_spend(genesis_coinbase(), 4'999'999'000));
    const auto popped = query.pop_confirmed();
    const auto reverted = query.set_pool_block(link, false);

    BC_REQUIRE(pooled);
    BC_REQUIRE(!link.is_terminal());
    BC_REQUIRE(popped);
    BC_REQUIRE(reverted);
    BC_REQUIRE_EQUAL(query.get_pool_top().value, 1u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_unvalidated__block__pool_top_parent)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto link = archive_confirm(query, to_spend(genesis_coinbase(), 4'999'999'000));
    const auto popped = query.pop_confirmed();
    size_t marked{};
    const auto unvalidated = query.set_pool_unvalidated(marked, link);

    BC_REQUIRE(!link.is_terminal());
    BC_REQUIRE(popped);
    BC_REQUIRE(unvalidated);
    BC_REQUIRE_EQUAL(query.get_pool_top().value, 0u);
}

BC_AUTO_TEST_CASE(query_pool__set_pool_block__disabled__pool_top_terminal)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    size_t marked{};
    BC_REQUIRE(query.set_pool_block(query.to_header(test::genesis.hash()), true));
    BC_REQUIRE(query.set_pool_unvalidated(marked, query.to_header(test::genesis.hash())));
    BC_REQUIRE(query.get_pool_top().is_terminal());
}

BC_AUTO_TEST_SUITE_END()
