/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

// construct
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__construct__default_configuration__referenced)
{
    const settings configuration{};
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(&instance.configuration(), &configuration);
}

BOOST_AUTO_TEST_CASE(store__is_dirty__uninitialized__true)
{
    const settings configuration{};
    store<map> instance{ configuration };
    BOOST_REQUIRE(instance.is_dirty());
}

BOOST_AUTO_TEST_CASE(store__is_dirty__initialized___false)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    query<store<map>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    BOOST_REQUIRE(!instance.is_dirty());
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__set_dirty__initialized__is_dirty)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    query<store<map>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    BOOST_REQUIRE(!instance.is_dirty());
    instance.set_dirty();
    BOOST_REQUIRE(instance.is_dirty());
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__is_dirty__open__false)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    query<store<map>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    BOOST_REQUIRE(!instance.is_dirty());
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__is_dirty__open_add_header__false)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    query<store<map>> query_{ instance };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(query_.initialize(test::genesis));
    BOOST_REQUIRE(query_.set(system::chain::header{}, context{}, false));
    BOOST_REQUIRE(!instance.is_dirty());
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__is_dirty__open_with_two_headers__true)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;

    store<map> instance1{ configuration };
    query<store<map>> query1_{ instance1 };
    BOOST_REQUIRE(!instance1.create(test::events));
    BOOST_REQUIRE(query1_.initialize(test::genesis));
    BOOST_REQUIRE(query1_.set(system::chain::header{}, context{}, false));
    BOOST_REQUIRE(!instance1.close(test::events));

    store<map> instance2{ configuration };
    BOOST_REQUIRE(!instance1.open(test::events));
    BOOST_REQUIRE(instance2.is_dirty());
    BOOST_REQUIRE(!instance1.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__paths__default_configuration__expected)
{
    const settings configuration{};
    test::map_store instance{ configuration };

    /// Archive.
    BOOST_REQUIRE_EQUAL(instance.header_head_file(), "bitcoin/heads/archive_header.head");
    BOOST_REQUIRE_EQUAL(instance.header_body_file(), "bitcoin/archive_header.data");
    BOOST_REQUIRE_EQUAL(instance.input_head_file(), "bitcoin/heads/archive_input.head");
    BOOST_REQUIRE_EQUAL(instance.input_body_file(), "bitcoin/archive_input.data");
    BOOST_REQUIRE_EQUAL(instance.output_head_file(), "bitcoin/heads/archive_output.head");
    BOOST_REQUIRE_EQUAL(instance.output_body_file(), "bitcoin/archive_output.data");
    BOOST_REQUIRE_EQUAL(instance.point_head_file(), "bitcoin/heads/archive_point.head");
    BOOST_REQUIRE_EQUAL(instance.point_body_file(), "bitcoin/archive_point.data");
    BOOST_REQUIRE_EQUAL(instance.outs_head_file(), "bitcoin/heads/archive_outs.head");
    BOOST_REQUIRE_EQUAL(instance.outs_body_file(), "bitcoin/archive_outs.data");
    BOOST_REQUIRE_EQUAL(instance.tx_head_file(), "bitcoin/heads/archive_tx.head");
    BOOST_REQUIRE_EQUAL(instance.tx_body_file(), "bitcoin/archive_tx.data");
    BOOST_REQUIRE_EQUAL(instance.txs_head_file(), "bitcoin/heads/archive_txs.head");
    BOOST_REQUIRE_EQUAL(instance.txs_body_file(), "bitcoin/archive_txs.data");

    /// Index.
    BOOST_REQUIRE_EQUAL(instance.candidate_head_file(), "bitcoin/heads/index_candidate.head");
    BOOST_REQUIRE_EQUAL(instance.candidate_body_file(), "bitcoin/index_candidate.data");
    BOOST_REQUIRE_EQUAL(instance.confirmed_head_file(), "bitcoin/heads/index_confirmed.head");
    BOOST_REQUIRE_EQUAL(instance.confirmed_body_file(), "bitcoin/index_confirmed.data");
    BOOST_REQUIRE_EQUAL(instance.strong_tx_head_file(), "bitcoin/heads/index_strong.head");
    BOOST_REQUIRE_EQUAL(instance.strong_tx_body_file(), "bitcoin/index_strong.data");

    /// Cache.
    BOOST_REQUIRE_EQUAL(instance.duplicate_head_file(), "bitcoin/heads/cache_duplicate.head");
    BOOST_REQUIRE_EQUAL(instance.duplicate_body_file(), "bitcoin/cache_duplicate.data");
    BOOST_REQUIRE_EQUAL(instance.prevout_head_file(), "bitcoin/heads/cache_prevout.head");
    BOOST_REQUIRE_EQUAL(instance.prevout_body_file(), "bitcoin/cache_prevout.data");
    BOOST_REQUIRE_EQUAL(instance.validated_tx_head_file(), "bitcoin/heads/validated_tx.head");
    BOOST_REQUIRE_EQUAL(instance.validated_tx_body_file(), "bitcoin/validated_tx.data");

    /// Option.
    BOOST_REQUIRE_EQUAL(instance.address_head_file(), "bitcoin/heads/option_address.head");
    BOOST_REQUIRE_EQUAL(instance.address_body_file(), "bitcoin/option_address.data");
    BOOST_REQUIRE_EQUAL(instance.filter_bk_head_file(), "bitcoin/heads/option_filter_bk.head");
    BOOST_REQUIRE_EQUAL(instance.filter_bk_body_file(), "bitcoin/option_filter_bk.data");
    BOOST_REQUIRE_EQUAL(instance.filter_tx_head_file(), "bitcoin/heads/option_filter_tx.head");
    BOOST_REQUIRE_EQUAL(instance.filter_tx_body_file(), "bitcoin/option_filter_tx.data");

    /// Lock.
    BOOST_REQUIRE_EQUAL(instance.flush_lock_file(), "bitcoin/flush.lock");
    BOOST_REQUIRE_EQUAL(instance.process_lock_file(), "bitcoin/process.lock");
}

// get_transactor
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__get_transactor__always__share_locked)
{
    const settings configuration{};
    test::map_store instance{ configuration };
    auto transactor = instance.get_transactor();
    BOOST_REQUIRE(transactor);
    BOOST_REQUIRE(!instance.transactor_mutex().try_lock());
    BOOST_REQUIRE(instance.transactor_mutex().try_lock_shared());
}

BOOST_AUTO_TEST_SUITE_END()
