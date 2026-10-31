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

// construct
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__construct__default_configuration__referenced)
{
    const settings configuration{ test::database_settings() };
    test::map_store instance{ configuration };
    BC_REQUIRE_EQUAL(&instance.configuration(), &configuration);
}

BC_AUTO_TEST_CASE(store__is_dirty__uninitialized__true)
{
    const settings configuration{ test::database_settings() };
    store<database::mmap> instance{ configuration };
    BC_REQUIRE(instance.is_dirty());
}

BC_AUTO_TEST_CASE(store__is_dirty__initialized___false)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(!instance.is_dirty());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__set_dirty__initialized__is_dirty)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(!instance.is_dirty());
    instance.set_dirty();
    BC_REQUIRE(instance.is_dirty());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__is_pooling__default__false)
{
    const settings configuration{ test::database_settings() };
    store<database::mmap> instance{ configuration };
    BC_REQUIRE(!instance.is_pooling());
}

BC_AUTO_TEST_CASE(store__set_pooling__initialized__is_pooling_and_dirty)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(!instance.is_pooling());
    BC_REQUIRE(!instance.is_dirty());
    instance.set_pooling();
    BC_REQUIRE(instance.is_pooling());
    BC_REQUIRE(instance.is_dirty());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__is_dirty__open__false)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(!instance.is_dirty());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__is_dirty__open_add_header__false)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(query_.set(system::chain::header{}, context{}, {}, false));
    BC_REQUIRE(!instance.is_dirty());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__is_dirty__open_with_two_headers__true)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    BC_REQUIRE(query_.set(system::chain::header{}, context{}, {}, false));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(instance.is_dirty());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__paths__default_configuration__expected)
{
    const settings configuration{ test::database_settings() };
    test::map_store instance{ configuration };

    /// Archive.
    BC_REQUIRE_EQUAL(instance.header_head_file(), "bitcoin/heads/archive_header.head");
    BC_REQUIRE_EQUAL(instance.header_body_file(), "bitcoin/archive_header.data");
    BC_REQUIRE_EQUAL(instance.input_head_file(), "bitcoin/heads/archive_input.head");
    BC_REQUIRE_EQUAL(instance.input_body_file(), "bitcoin/archive_input.data");
    BC_REQUIRE_EQUAL(instance.output_head_file(), "bitcoin/heads/archive_output.head");
    BC_REQUIRE_EQUAL(instance.output_body_file(), "bitcoin/archive_output.data");
    BC_REQUIRE_EQUAL(instance.ins_head_file(), "bitcoin/heads/archive_ins.head");
    BC_REQUIRE_EQUAL(instance.ins_body_file(), "bitcoin/archive_ins_point.data");
    BC_REQUIRE_EQUAL(instance.outs_head_file(), "bitcoin/heads/archive_outs.head");
    BC_REQUIRE_EQUAL(instance.outs_body_file(), "bitcoin/archive_outs_address.data");
    BC_REQUIRE_EQUAL(instance.tx_head_file(), "bitcoin/heads/archive_tx.head");
    BC_REQUIRE_EQUAL(instance.tx_body_file(), "bitcoin/archive_tx.data");
    BC_REQUIRE_EQUAL(instance.txs_head_file(), "bitcoin/heads/archive_txs.head");
    BC_REQUIRE_EQUAL(instance.txs_body_file(), "bitcoin/archive_txs.data");

    /// Index.
    BC_REQUIRE_EQUAL(instance.candidate_head_file(), "bitcoin/heads/index_candidate.head");
    BC_REQUIRE_EQUAL(instance.confirmed_head_file(), "bitcoin/heads/index_confirmed.head");
    BC_REQUIRE_EQUAL(instance.strong_head_file(), "bitcoin/heads/index_strong.head");
    BC_REQUIRE_EQUAL(instance.duplicate_head_file(), "bitcoin/heads/index_duplicate.head");
    BC_REQUIRE_EQUAL(instance.duplicate_body_file(), "bitcoin/index_duplicate.data");
    BC_REQUIRE_EQUAL(instance.state_head_file(), "bitcoin/heads/index_state.head");
    BC_REQUIRE_EQUAL(instance.wtxid_head_file(), "bitcoin/heads/index_wtxid.head");
    BC_REQUIRE_EQUAL(instance.wtxid_body_file(), "bitcoin/index_wtxid.data");

    /// Cache.
    BC_REQUIRE_EQUAL(instance.prevout_head_file(), "bitcoin/heads/cache_prevout.head");
    BC_REQUIRE_EQUAL(instance.prevout_body_file(), "bitcoin/cache_prevout.data");
    BC_REQUIRE_EQUAL(instance.pool_head_file(), "bitcoin/heads/cache_pool.head");
    BC_REQUIRE_EQUAL(instance.pool_body_file(), "bitcoin/cache_pool_context.data");

    /// Option.
    BC_REQUIRE_EQUAL(instance.filter_bk_head_file(), "bitcoin/heads/option_filter_bk.head");
    BC_REQUIRE_EQUAL(instance.filter_bk_body_file(), "bitcoin/option_filter_bk.data");
    BC_REQUIRE_EQUAL(instance.filter_tx_head_file(), "bitcoin/heads/option_filter_tx.head");
    BC_REQUIRE_EQUAL(instance.filter_tx_body_file(), "bitcoin/option_filter_tx.data");

    /// Lock.
    BC_REQUIRE_EQUAL(instance.flush_lock_file(), "bitcoin/flush.lock");
    BC_REQUIRE_EQUAL(instance.process_lock_file(), "bitcoin/process.lock");
}

// get_transactor
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__get_transactor__always__share_locked)
{
    const settings configuration{ test::database_settings() };
    test::map_store instance{ configuration };
    auto transactor = instance.get_transactor();
    BC_REQUIRE(transactor);
    BC_REQUIRE(!instance.transactor_mutex().try_lock());
    BC_REQUIRE(instance.transactor_mutex().try_lock_shared());
}

// set_pooling
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__set_pooling__created__pooling)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.is_pooling());
    instance.set_pooling();
    BC_REQUIRE(instance.is_pooling());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__set_pooling__twice__remains_pooling)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    instance.set_pooling();
    instance.set_pooling();
    BC_REQUIRE(instance.is_pooling());
    BC_REQUIRE(!instance.close(test::events));
}

// set_current
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__set_current__created__not_current)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.is_current_());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__set_current__changed__applied_to_heads)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    instance.set_current_(true);
    instance.set_current_(false);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__set_current__unchanged__no_effect)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    instance.set_current_(false);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_SUITE_END()
