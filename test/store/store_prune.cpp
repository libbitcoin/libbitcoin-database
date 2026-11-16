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

BC_AUTO_TEST_CASE(store__prune__pool__unchanged)
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

    pooled_tx pooled{};
    pooled.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(pooled, link, validated_context), error::success);
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

BC_AUTO_TEST_CASE(store__prune__spends__unchanged)
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
    BC_REQUIRE_EQUAL(query_.spends_records(), 3u);
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

BC_AUTO_TEST_SUITE_END()
