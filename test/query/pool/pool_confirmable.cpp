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
using test::archive_pool;
using test::archive_confirm;
using test::pool_populated;

// is_confirmable_spent

BC_AUTO_TEST_CASE(query_pool__is_confirmable_spent__pooled_spender__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(!query.is_confirmable_spent(query.to_output(parent.hash(false), 0)));

    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(query.is_confirmable_spent(query.to_output(parent.hash(false), 0)));
    BC_REQUIRE(!query.is_confirmable_spent(query.to_output(child.hash(false), 0)));
}

BC_AUTO_TEST_CASE(query_pool__is_confirmable_spent__conflicted_spender__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());
    BC_REQUIRE(!query.is_confirmable_spent(query.to_output(parent.hash(false), 0)));
}

// find_confirmable_spender

BC_AUTO_TEST_CASE(query_pool__find_confirmable_spender__pooled_spender__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    const point prevout{ parent.hash(false), 0 };
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(query.find_confirmable_spender(prevout).is_terminal());

    BC_REQUIRE(archive_pool(query, child));
    const auto spender = query.find_confirmable_spender(prevout);
    BC_REQUIRE(query.to_input_tx(spender) == query.to_tx(child.hash(false)));
}

BC_AUTO_TEST_CASE(query_pool__find_confirmable_spender__conflicted_spender__terminal)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    const point prevout{ parent.hash(false), 0 };
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());
    BC_REQUIRE(query.find_confirmable_spender(prevout).is_terminal());
}

// is_confirmable_tx

BC_AUTO_TEST_CASE(query_pool__is_confirmable_tx__unconflicted_chain__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(query.is_confirmable_tx(query.to_tx(parent.hash(false))));
    BC_REQUIRE(query.is_confirmable_tx(query.to_tx(child.hash(false))));
}

BC_AUTO_TEST_CASE(query_pool__is_confirmable_tx__confirmed__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto tx = to_spend(genesis_coinbase(), 1u);
    BC_REQUIRE(archive_pool(query, tx));
    BC_REQUIRE(!archive_confirm(query, tx).is_terminal());
    BC_REQUIRE(!query.is_confirmable_tx(query.to_tx(tx.hash(false))));
    BC_REQUIRE(!query.is_confirmable_tx(tx_link{ 0 }));
}

BC_AUTO_TEST_CASE(query_pool__is_confirmable_tx__conflicted_and_child__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, conflicted));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 1u)).is_terminal());
    BC_REQUIRE(!query.is_confirmable_tx(query.to_tx(conflicted.hash(false))));
    BC_REQUIRE(!query.is_confirmable_tx(query.to_tx(child.hash(false))));
}

// get_confirmable_txs

BC_AUTO_TEST_CASE(query_pool__get_confirmable_txs__empty__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    tx_links out{ 42 };
    BC_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BC_REQUIRE(out.empty());
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_txs__disabled__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    tx_links out{ 42 };
    BC_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BC_REQUIRE(out.empty());
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_txs__parent_and_child__pool_order)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));

    tx_links out{};
    BC_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BC_REQUIRE_EQUAL(out.size(), 2u);
    BC_REQUIRE_EQUAL(out.front(), query.to_tx(parent.hash(false)).value);
    BC_REQUIRE_EQUAL(out.back(), query.to_tx(child.hash(false)).value);
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_txs__conflicted_and_child__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, conflicted));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());

    tx_links out{ 42 };
    BC_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BC_REQUIRE(out.empty());
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_txs__confirmed_parent__child_only)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, parent).is_terminal());

    tx_links out{};
    BC_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BC_REQUIRE_EQUAL(out.size(), 1u);
    BC_REQUIRE_EQUAL(out.front(), query.to_tx(child.hash(false)).value);
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_txs__insufficient_context__excluded)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto other = to_spend(genesis_coinbase(), 4'999'997'000);
    const auto later = to_spend(genesis_coinbase(), 4'999'999'000);
    BC_REQUIRE(pool_populated(query, other, 7));
    BC_REQUIRE(pool_populated(query, later, 9));

    size_t count{};
    BC_REQUIRE(query.set_pool_context(count, context{ bip113, 8, 9 }));
    BC_REQUIRE_EQUAL(count, 1u);

    tx_links out{};
    BC_REQUIRE_EQUAL(query.get_confirmable_txs(out), error::success);
    BC_REQUIRE_EQUAL(out.size(), 1u);
    BC_REQUIRE_EQUAL(out.front(), query.to_tx(other.hash(false)).value);
}

// get_confirmable_fees

BC_AUTO_TEST_CASE(query_pool__get_confirmable_fees__empty__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    fee_rates out{ fee_rate{} };
    BC_REQUIRE_EQUAL(query.get_confirmable_fees(out), error::success);
    BC_REQUIRE(out.empty());
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_fees__parent_and_child__totals)
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
    BC_REQUIRE(pool_populated(query, parent, 8));
    BC_REQUIRE(pool_populated(query, child, 9));

    fee_rates out{};
    BC_REQUIRE_EQUAL(query.get_confirmable_fees(out), error::success);
    BC_REQUIRE_EQUAL(out.size(), 2u);
    BC_REQUIRE_EQUAL(out.at(0).fee, 1'000u);
    BC_REQUIRE_EQUAL(out.at(0).bytes, parent.virtual_size());
    BC_REQUIRE_EQUAL(out.at(0).pooled_height, 8u);
    BC_REQUIRE_EQUAL(out.at(1).fee, 9'000u);
    BC_REQUIRE_EQUAL(out.at(1).bytes, child.virtual_size());
    BC_REQUIRE_EQUAL(out.at(1).pooled_height, 9u);
}

BC_AUTO_TEST_CASE(query_pool__get_confirmable_fees__conflicted_and_child__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto conflicted = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(conflicted.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, conflicted));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());

    fee_rates out{ fee_rate{} };
    BC_REQUIRE_EQUAL(query.get_confirmable_fees(out), error::success);
    BC_REQUIRE(out.empty());
}

// to_confirmable_parents

BC_AUTO_TEST_CASE(query_pool__to_confirmable_parents__pooled_parent__parent)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));

    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    BC_REQUIRE(query.to_confirmable_parents(parent_link).empty());
    BC_REQUIRE_EQUAL(query.to_confirmable_parents(child_link), tx_links{ parent_link.value });
}

// to_confirmable_children

BC_AUTO_TEST_CASE(query_pool__to_confirmable_children__pooled_child__child)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));

    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    BC_REQUIRE_EQUAL(query.to_confirmable_children(parent_link), tx_links{ child_link.value });
    BC_REQUIRE(query.to_confirmable_children(child_link).empty());
}

BC_AUTO_TEST_CASE(query_pool__to_confirmable_children__conflicted_child__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(!archive_confirm(query, to_spend(genesis_coinbase(), 3u)).is_terminal());

    const auto parent_link = query.to_tx(parent.hash(false));
    BC_REQUIRE(query.to_confirmable_children(parent_link).empty());
}

// to_confirmable_ancestors

BC_AUTO_TEST_CASE(query_pool__to_confirmable_ancestors__pooled_chain__ancestors)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    const auto grandchild = to_spend(child.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(archive_pool(query, grandchild));

    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    const auto grandchild_link = query.to_tx(grandchild.hash(false));
    BC_REQUIRE(query.to_confirmable_ancestors(parent_link).empty());

    const auto ancestors = query.to_confirmable_ancestors(grandchild_link);
    BC_REQUIRE_EQUAL(ancestors.size(), two);
    BC_REQUIRE(system::contains(ancestors, parent_link.value));
    BC_REQUIRE(system::contains(ancestors, child_link.value));
}

// to_confirmable_descendants

BC_AUTO_TEST_CASE(query_pool__to_confirmable_descendants__pooled_chain__descendants)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    const auto grandchild = to_spend(child.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));
    BC_REQUIRE(archive_pool(query, grandchild));

    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    const auto grandchild_link = query.to_tx(grandchild.hash(false));
    BC_REQUIRE(query.to_confirmable_descendants(grandchild_link).empty());

    const auto descendants = query.to_confirmable_descendants(parent_link);
    BC_REQUIRE_EQUAL(descendants.size(), two);
    BC_REQUIRE(system::contains(descendants, child_link.value));
    BC_REQUIRE(system::contains(descendants, grandchild_link.value));
}

// get_ancestor_totals

BC_AUTO_TEST_CASE(query_pool__get_ancestor_totals__pooled_parent__both)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));

    fee_totals out{};
    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    BC_REQUIRE(query.get_ancestor_totals(out, parent_link));
    BC_REQUIRE_EQUAL(out.count, one);
    BC_REQUIRE_EQUAL(out.bytes, parent.virtual_size());

    BC_REQUIRE(query.get_ancestor_totals(out, child_link));
    BC_REQUIRE_EQUAL(out.count, two);
    BC_REQUIRE_EQUAL(out.bytes, parent.virtual_size() + child.virtual_size());
}

// get_descendant_totals

BC_AUTO_TEST_CASE(query_pool__get_descendant_totals__pooled_child__both)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 2u);
    const auto child = to_spend(parent.hash(false), 1u);
    BC_REQUIRE(archive_pool(query, parent));
    BC_REQUIRE(archive_pool(query, child));

    fee_totals out{};
    const auto parent_link = query.to_tx(parent.hash(false));
    const auto child_link = query.to_tx(child.hash(false));
    BC_REQUIRE(query.get_descendant_totals(out, parent_link));
    BC_REQUIRE_EQUAL(out.count, two);
    BC_REQUIRE_EQUAL(out.bytes, parent.virtual_size() + child.virtual_size());

    BC_REQUIRE(query.get_descendant_totals(out, child_link));
    BC_REQUIRE_EQUAL(out.count, one);
    BC_REQUIRE_EQUAL(out.bytes, child.virtual_size());
}

BC_AUTO_TEST_SUITE_END()
