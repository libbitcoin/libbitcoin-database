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

BOOST_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// prune
// ----------------------------------------------------------------------------
// Empty store asserts so create and initialize.

BOOST_AUTO_TEST_CASE(store__prune__initialized__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    BOOST_REQUIRE(!instance.prune(test::events));

    // The retained snapshot is post-prune (the trim strands nothing).
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
    BOOST_REQUIRE_EQUAL(query_.prevout_body_size(), zero);
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__prune__faulted_restore__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    BOOST_REQUIRE(!instance.prune(test::events));
    BOOST_REQUIRE(!instance.close(test::events));

    // Simulate power fault (flush lock persists), restore from post-prune.
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(!instance.restore(test::events));
    BOOST_REQUIRE(query_.is_initialized());
    BOOST_REQUIRE_EQUAL(query_.prevout_body_size(), zero);
    BOOST_REQUIRE(!instance.close(test::events));
}

constexpr database::context validated_context
{
    system::chain::flags::bip113_rule, 8, 9
};

BOOST_AUTO_TEST_CASE(store__prune__validated_tx__cleared)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    test::tx_spend_one_hash.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query_.set_tx_state(1, test::tx_spend_one_hash, validated_context));
    BOOST_REQUIRE(!instance.prune(test::events));
    BOOST_REQUIRE_EQUAL(query_.validated_tx_body_size(), zero);

    tx_state state{};
    state.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query_.get_tx_state(state, 1, validated_context), error::unvalidated);
    BOOST_REQUIRE(query_.set_tx_state(2, test::tx_spend_one_hash, validated_context));
    BOOST_REQUIRE_EQUAL(query_.get_tx_state(state, 2, validated_context), error::success);
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__prune__faulted_restore__validated_tx_cleared)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    test::tx_spend_one_hash.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query_.set_tx_state(1, test::tx_spend_one_hash, validated_context));
    BOOST_REQUIRE(!instance.prune(test::events));
    BOOST_REQUIRE(!instance.close(test::events));

    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(!instance.restore(test::events));
    BOOST_REQUIRE_EQUAL(query_.validated_tx_body_size(), zero);

    tx_state state{};
    state.prevouts.resize(one);
    BOOST_REQUIRE_EQUAL(query_.get_tx_state(state, 1, validated_context), error::unvalidated);
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__prune__spends__cleared)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));

    const table::spends::put_refs::parents parents{ 1, 2, 3 };
    BOOST_REQUIRE(instance.spends.put(table::spends::put_refs{ {}, parents }));
    BOOST_REQUIRE_EQUAL(query_.spends_records(), 3u);
    BOOST_REQUIRE(!instance.prune(test::events));
    BOOST_REQUIRE_EQUAL(query_.spends_records(), zero);
    BOOST_REQUIRE_EQUAL(query_.spends_body_size(), zero);
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__prune__faulted_restore__spends_cleared)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));

    const table::spends::put_refs::parents parents{ 1, 2, 3 };
    BOOST_REQUIRE(instance.spends.put(table::spends::put_refs{ {}, parents }));
    BOOST_REQUIRE(!instance.prune(test::events));
    BOOST_REQUIRE(!instance.close(test::events));

    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(!instance.restore(test::events));
    BOOST_REQUIRE_EQUAL(query_.spends_records(), zero);
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_SUITE_END()
