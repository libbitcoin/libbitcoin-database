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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/map_store.hpp"

// these include the slow tests (mmap)

BC_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// prune
// ----------------------------------------------------------------------------
// Empty store asserts so create and initialize.

BC_AUTO_TEST_CASE(store__prune__initialized__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(!instance.prune(test::events));

    // The retained snapshot is post-prune (the trim strands nothing).
    BC_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
    BC_REQUIRE_EQUAL(query_.prevout_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__faulted_restore__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE(!instance.close(test::events));

    // Simulate power fault (flush lock persists), restore from post-prune.
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE(query_.is_initialized());
    BC_REQUIRE_EQUAL(query_.prevout_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

constexpr database::context validated_context
{
    system::chain::flags::bip113_rule, 8, 9
};

BC_AUTO_TEST_CASE(store__prune__pool__cleared)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    test::tx_spend_one_hash.inputs_ptr()->front()->metadata.parent_tx = 42;
    BC_REQUIRE(query_.set(test::tx_spend_one_hash));

    const auto link = query_.to_tx(test::tx_spend_one_hash.hash(false));
    BC_REQUIRE(query_.set_pooled(link, test::tx_spend_one_hash, validated_context, 0));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(pooled, link, validated_context), error::unvalidated);
    BC_REQUIRE(query_.set_pooled(2, test::tx_spend_one_hash, validated_context, 0));
    BC_REQUIRE_EQUAL(query_.get_pooled(pooled, 2, validated_context), error::success);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__faulted_restore__pool_cleared)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    test::tx_spend_one_hash.inputs_ptr()->front()->metadata.parent_tx = 42;
    BC_REQUIRE(query_.set(test::tx_spend_one_hash));

    const auto link = query_.to_tx(test::tx_spend_one_hash.hash(false));
    BC_REQUIRE(query_.set_pooled(link, test::tx_spend_one_hash, validated_context, 0));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE(!instance.close(test::events));

    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(pooled, link, validated_context), error::unvalidated);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__spends__cleared)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));

    const table::spends::put_refs::parents parents{ 1, 2, 3 };
    BC_REQUIRE(instance.spends.put(table::spends::put_refs{ {}, parents }));
    BC_REQUIRE_EQUAL(query_.spends_records(), 3u);
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(query_.spends_records(), zero);
    BC_REQUIRE_EQUAL(query_.spends_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__faulted_restore__spends_cleared)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));

    const table::spends::put_refs::parents parents{ 1, 2, 3 };
    BC_REQUIRE(instance.spends.put(table::spends::put_refs{ {}, parents }));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE(!instance.close(test::events));

    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE_EQUAL(query_.spends_records(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

// compaction
// ----------------------------------------------------------------------------

using spend_t = system::chain::transaction;

static spend_t spend(const system::hash_digest& hash, uint64_t value) NOEXCEPT
{
    const system::chain::point point{ hash, 0 };
    const system::chain::input input{ point, system::chain::script{}, max_uint32 };
    const system::chain::output output{ value, system::chain::script{} };
    return { 1u, system::chain::inputs{ input }, system::chain::outputs{ output }, 0u };
}

static const system::hash_digest& genesis_coinbase() NOEXCEPT
{
    static const auto hash = test::genesis.transactions_ptr()->front()->hash(false);
    return hash;
}

// Archive the tx and pool it under the validated context.
static bool pool(query<store<database::mmap>>& query_, const spend_t& tx) NOEXCEPT
{
    const auto stored = query_.set(tx);
    const auto link = query_.to_tx(tx.hash(false));
    return stored && query_.set_pooled(link, tx, validated_context, 0);
}

// Confirm a block of a coinbase and the tx above genesis (a pooled tx is
// associated by its pooled link, as the store is pooling).
static bool confirm(query<store<database::mmap>>& query_, const spend_t& tx) NOEXCEPT
{
    const system::chain::point null{ system::null_hash, system::chain::point::null_index };
    const system::chain::input input{ null, system::chain::script{}, max_uint32 };
    const system::chain::output output{ 42u, system::chain::script{} };
    const spend_t coinbase{ 1u, system::chain::inputs{ input }, system::chain::outputs{ output }, 0u };
    const system::chain::header header{ 1u, test::genesis.hash(), system::null_hash, 1u, 2u, 3u };
    const system::chain::block block{ header, { coinbase, tx } };
    const auto stored = query_.set(block, context{ 0, 1, 0 }, {}, false, true);
    const auto link = query_.to_header(block.hash());
    const auto candidate = stored && query_.push_candidate(link);
    return candidate && query_.push_confirmed(link, true);
}

static code pooled(query<store<database::mmap>>& query_, const spend_t& tx) NOEXCEPT
{
    pooled_tx out{};
    out.prevouts.resize(one);
    return query_.get_pooled(out, query_.to_tx(tx.hash(false)), validated_context);
}

BC_AUTO_TEST_CASE(store__prune__unconfirmed_pooled__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto unconfirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, unconfirmed));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::compact));

    pooled_tx out{};
    out.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(out, query_.to_tx(unconfirmed.hash(false)), validated_context), error::success);
    BC_REQUIRE_EQUAL(out.prevouts.front().parent, 0u);
    BC_REQUIRE(out.prevouts.front().coinbase);
    BC_REQUIRE_EQUAL(query_.spends_records(), one);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__confirmed_pooled__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, confirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query_.spends_records(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__conflicting_pooled__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(confirm(query_, spend(genesis_coinbase(), 1u)));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__descendant_of_conflicting__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto descendant = spend(conflict.hash(false), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, descendant));
    BC_REQUIRE(confirm(query_, spend(genesis_coinbase(), 1u)));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, descendant), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__descendant_of_retained__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto parent = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 1u);
    BC_REQUIRE(pool(query_, parent));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, parent), error::success);

    pooled_tx out{};
    out.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(out, query_.to_tx(child.hash(false)), validated_context), error::success);
    BC_REQUIRE_EQUAL(out.prevouts.front().parent, query_.to_tx(parent.hash(false)));
    BC_REQUIRE(!out.prevouts.front().coinbase);
    BC_REQUIRE_EQUAL(query_.spends_records(), two);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__retained_and_dropped__only_retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, confirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, retained), error::success);
    BC_REQUIRE_EQUAL(query_.spends_records(), one);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__child_pooled_before_parent__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto parent = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 1u);
    BC_REQUIRE(query_.set(parent));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(query_.set_pooled(query_.to_tx(parent.hash(false)), parent, validated_context, 0));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, parent), error::success);
    BC_REQUIRE_EQUAL(pooled(query_, child), error::success);
    BC_REQUIRE_EQUAL(query_.spends_records(), two);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), two);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__child_of_unpooled_parent__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto parent = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 1u);
    BC_REQUIRE(query_.set(parent));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, child), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__retained__found_by_witness_hash)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(query_.wtxid_records(), one);
    BC_REQUIRE_EQUAL(query_.to_witness_tx(retained.hash(true)), query_.to_tx(retained.hash(false)));
    BC_REQUIRE(!query_.is_pooled(query_.to_tx(conflict.hash(false))));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__residual_compact_empty_pool__deleted)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::compact));
    BC_REQUIRE(test::create(configuration.path / schema::dir::compact / "cache_pool.head"));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::compact));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__residual_compact_pooled__deleted_retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto unconfirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, unconfirmed));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::compact));
    BC_REQUIRE(test::create(configuration.path / schema::dir::compact / "cache_pool.head"));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::compact));
    BC_REQUIRE_EQUAL(pooled(query_, unconfirmed), error::success);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__prune__faulted_restore__retained_pool_empty)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto unconfirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, unconfirmed));
    BC_REQUIRE(!instance.prune(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, unconfirmed), error::success);
    BC_REQUIRE(!instance.close(test::events));

    // The restore point is the prune snapshot, which records an empty pool.
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, unconfirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query_.wtxid_body_size(), zero);
    BC_REQUIRE(!query_.is_pooled(query_.to_tx(unconfirmed.hash(false))));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_SUITE_END()
