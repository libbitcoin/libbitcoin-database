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
#include "../mocks/chunk_store.hpp"

BC_FIXTURE_TEST_SUITE(query_extent_tests, test::directory_setup_fixture)

BC_AUTO_TEST_CASE(query__is_full__chunk_store__false)
{
    const settings configuration{ test::database_settings() };
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    BC_REQUIRE(!query.is_full());
}

BC_AUTO_TEST_CASE(query_extent__body_sizes__genesis__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    BC_REQUIRE_EQUAL(query.header_body_size(), schema::header::minrow);
    BC_REQUIRE_EQUAL(query.output_body_size(), 81u);
    BC_REQUIRE_EQUAL(query.input_body_size(), 79u);
    BC_REQUIRE_EQUAL(query.ins_body_size(), schema::ins::minrow + schema::ins_sequence::minrow);
    BC_REQUIRE_EQUAL(query.outs_body_size(), schema::address::minrow + schema::outs::minrow);
    BC_REQUIRE_EQUAL(query.txs_body_size(), schema::txs::minrow);
    BC_REQUIRE_EQUAL(query.tx_body_size(), schema::transaction::minrow);

    BC_REQUIRE_EQUAL(query.candidate_body_size(), zero);
    BC_REQUIRE_EQUAL(query.confirmed_body_size(), zero);
    BC_REQUIRE_EQUAL(query.strong_body_size(), zero);
    BC_REQUIRE_EQUAL(query.scan_body_size(), zero);
    BC_REQUIRE_EQUAL(query.ecdsa0_body_size(), zero);
    BC_REQUIRE_EQUAL(query.schnorr0_body_size(), zero);
    BC_REQUIRE_EQUAL(query.duplicate_body_size(), zero);
    BC_REQUIRE_EQUAL(query.prevalid0_body_size(), zero);
    BC_REQUIRE_EQUAL(query.presilent0_body_size(), zero);
    BC_REQUIRE_EQUAL(query.prevout_body_size(), zero);
    BC_REQUIRE_EQUAL(query.state_body_size(), zero);
    BC_REQUIRE_EQUAL(query.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query.spends_body_size(), zero);
    BC_REQUIRE_EQUAL(query.wtxid_body_size(), zero);
    BC_REQUIRE_EQUAL(query.filter_bk_body_size(), schema::filter_bk::minrow);
    BC_REQUIRE_EQUAL(query.filter_tx_body_size(), 5u);
}

BC_AUTO_TEST_CASE(query_extent__outs_body_size__address_disabled__puts_only)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.outs.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // The address spine is unbacked when the hash head is disabled.
    BC_REQUIRE(!query.address_enabled());
    BC_REQUIRE_EQUAL(query.outs_body_size(), schema::outs::minrow);
    BC_REQUIRE_EQUAL(query.outs_head_size(), schema::address::link::size);
}

BC_AUTO_TEST_CASE(query_extent__buckets__genesis__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    BC_REQUIRE_EQUAL(query.header_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.ins_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.outs_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.txs_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.tx_buckets(), 128u);

    BC_REQUIRE_EQUAL(query.candidate_buckets(), one);
    BC_REQUIRE_EQUAL(query.confirmed_buckets(), one);
    BC_REQUIRE_EQUAL(query.strong_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.state_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.duplicate_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.prevout_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.pool_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.wtxid_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.filter_tx_buckets(), 128u);
    BC_REQUIRE_EQUAL(query.filter_bk_buckets(), 128u);
}

BC_AUTO_TEST_CASE(query_extent__records__genesis__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    BC_REQUIRE_EQUAL(query.header_records(), one);
    BC_REQUIRE_EQUAL(query.ins_records(), one);
    BC_REQUIRE_EQUAL(query.outs_records(), one);
    BC_REQUIRE_EQUAL(query.tx_records(), one);

    BC_REQUIRE_EQUAL(query.scan_records(), zero);
    BC_REQUIRE_EQUAL(query.ecdsa0_records(), zero);
    BC_REQUIRE_EQUAL(query.schnorr0_records(), zero);
    BC_REQUIRE_EQUAL(query.duplicate_records(), zero);
    BC_REQUIRE_EQUAL(query.prevalid0_records(), zero);
    BC_REQUIRE_EQUAL(query.presilent0_records(), zero);
    BC_REQUIRE_EQUAL(query.pool_records(), zero);
    BC_REQUIRE_EQUAL(query.spends_records(), zero);
    BC_REQUIRE_EQUAL(query.wtxid_records(), zero);
    BC_REQUIRE_EQUAL(query.filter_bk_records(), one);
}

BC_AUTO_TEST_CASE(query_extent__input_output_count__genesis__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    BC_REQUIRE_EQUAL(query.input_count(0), one);
    BC_REQUIRE_EQUAL(query.output_count(0), one);
    BC_REQUIRE_EQUAL(query.put_counts(0).first, one);
    BC_REQUIRE_EQUAL(query.put_counts(0).second, one);

    BC_REQUIRE_EQUAL(query.input_count(1), zero);
    BC_REQUIRE_EQUAL(query.output_count(1), zero);
    BC_REQUIRE_EQUAL(query.put_counts(1).first, zero);
    BC_REQUIRE_EQUAL(query.put_counts(1).second, zero);
}

BC_AUTO_TEST_CASE(query_extent__optionals_enabled__default__true)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.address_enabled());
    BC_REQUIRE(query.filter_enabled());
}

BC_AUTO_TEST_CASE(query_extent__address_enabled__disabled__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.outs.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(!query.address_enabled());
    BC_REQUIRE(query.filter_enabled());
}

BC_AUTO_TEST_CASE(query_extent__filter_enabled__disabled__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.filter_tx.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.address_enabled());
    BC_REQUIRE(!query.filter_enabled());
}

// aggregate sizes
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_extent__archive_head_size__genesis__sum_of_tables)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    const auto expected = query.header_head_size()
        + query.output_head_size()
        + query.input_head_size()
        + query.ins_head_size()
        + query.outs_head_size()
        + query.txs_head_size()
        + query.tx_head_size();
    BC_REQUIRE_EQUAL(query.archive_head_size(), expected);
}

BC_AUTO_TEST_CASE(query_extent__archive_body_size__genesis__sum_of_tables)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    const auto expected = query.header_body_size()
        + query.output_body_size()
        + query.input_body_size()
        + query.ins_body_size()
        + query.outs_body_size()
        + query.txs_body_size()
        + query.tx_body_size();
    BC_REQUIRE_EQUAL(query.archive_body_size(), expected);
}

BC_AUTO_TEST_CASE(query_extent__archive_size__genesis__head_plus_body)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.archive_size(), query.archive_head_size() + query.archive_body_size());
}

BC_AUTO_TEST_CASE(query_extent__store_head_size__genesis__archive_plus_indexes)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    const auto indexes = query.candidate_head_size()
        + query.confirmed_head_size()
        + query.strong_head_size()
        + query.scan_head_size()
        + query.scan_prefix_head_size()
        + query.ecdsa0_head_size()
        + query.ecdsa1_head_size()
        + query.schnorr0_head_size()
        + query.schnorr1_head_size()
        + query.silent0_head_size()
        + query.silent1_head_size()
        + query.silent0_prefix_head_size()
        + query.silent1_prefix_head_size()
        + query.duplicate_head_size()
        + query.prevalid0_head_size()
        + query.prevalid1_head_size()
        + query.presilent0_head_size()
        + query.presilent1_head_size()
        + query.prevout_head_size()
        + query.state_head_size()
        + query.pool_head_size()
        + query.spends_head_size()
        + query.wtxid_head_size()
        + query.filter_bk_head_size()
        + query.filter_tx_head_size();
    BC_REQUIRE_EQUAL(query.store_head_size(), query.archive_head_size() + indexes);
}

BC_AUTO_TEST_CASE(query_extent__store_body_size__genesis__archive_plus_indexes)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    const auto indexes = query.candidate_body_size()
        + query.confirmed_body_size()
        + query.strong_body_size()
        + query.scan_body_size()
        + query.scan_prefix_body_size()
        + query.ecdsa0_body_size()
        + query.ecdsa1_body_size()
        + query.schnorr0_body_size()
        + query.schnorr1_body_size()
        + query.silent0_body_size()
        + query.silent1_body_size()
        + query.silent0_prefix_body_size()
        + query.silent1_prefix_body_size()
        + query.duplicate_body_size()
        + query.prevalid0_body_size()
        + query.prevalid1_body_size()
        + query.presilent0_body_size()
        + query.presilent1_body_size()
        + query.prevout_body_size()
        + query.state_body_size()
        + query.pool_body_size()
        + query.spends_body_size()
        + query.wtxid_body_size()
        + query.filter_bk_body_size()
        + query.filter_tx_body_size();
    BC_REQUIRE_EQUAL(query.store_body_size(), query.archive_body_size() + indexes);
}

BC_AUTO_TEST_CASE(query_extent__store_size__genesis__head_plus_body)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.store_size(), query.store_head_size() + query.store_body_size());
}

BC_AUTO_TEST_CASE(query_extent__store_size__archived_block__increases)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    const auto initial = query.store_body_size();
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE_GT(query.store_body_size(), initial);
}

BC_AUTO_TEST_SUITE_END()
