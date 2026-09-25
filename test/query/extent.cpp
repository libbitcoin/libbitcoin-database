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

BOOST_FIXTURE_TEST_SUITE(query_extent_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(query__is_full__chunk_store__false)
{
    const settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!query.is_full());
}

BOOST_AUTO_TEST_CASE(query_extent__body_sizes__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_body_size(), schema::header::minrow);
    BOOST_REQUIRE_EQUAL(query.output_body_size(), 81u);
    BOOST_REQUIRE_EQUAL(query.input_body_size(), 79u);
    BOOST_REQUIRE_EQUAL(query.ins_body_size(), schema::ins::minrow + schema::ins_sequence::minrow);
    BOOST_REQUIRE_EQUAL(query.outs_body_size(), schema::address::minrow + schema::outs::minrow);
    BOOST_REQUIRE_EQUAL(query.txs_body_size(), schema::txs::minrow);
    BOOST_REQUIRE_EQUAL(query.tx_body_size(), schema::transaction::minrow);

    BOOST_REQUIRE_EQUAL(query.candidate_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.confirmed_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.strong_tx_body_size(), schema::strong_tx::minrow);
    BOOST_REQUIRE_EQUAL(query.ecdsa_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.schnorr_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.silent_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.duplicate_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.prevalid_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.prevout_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.validated_bk_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.validated_tx_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.spends_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.filter_bk_body_size(), schema::filter_bk::minrow);
    BOOST_REQUIRE_EQUAL(query.filter_tx_body_size(), 5u);
}

BOOST_AUTO_TEST_CASE(query_extent__buckets__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.ins_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.outs_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.txs_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.tx_buckets(), 128u);

    BOOST_REQUIRE_EQUAL(query.strong_tx_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.duplicate_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.prevout_buckets(), 128);
    BOOST_REQUIRE_EQUAL(query.validated_tx_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.validated_bk_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.filter_tx_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.filter_bk_buckets(), 128u);
}

BOOST_AUTO_TEST_CASE(query_extent__records__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_records(), one);
    BOOST_REQUIRE_EQUAL(query.ins_records(), one);
    BOOST_REQUIRE_EQUAL(query.outs_records(), one);
    BOOST_REQUIRE_EQUAL(query.tx_records(), one);

    BOOST_REQUIRE_EQUAL(query.candidate_records(), one);
    BOOST_REQUIRE_EQUAL(query.confirmed_records(), one);
    BOOST_REQUIRE_EQUAL(query.strong_tx_records(), one);
    BOOST_REQUIRE_EQUAL(query.ecdsa_records(), zero);
    BOOST_REQUIRE_EQUAL(query.schnorr_records(), zero);
    BOOST_REQUIRE_EQUAL(query.silent_records(), zero);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), zero);
    BOOST_REQUIRE_EQUAL(query.prevalid_records(), zero);
    BOOST_REQUIRE_EQUAL(query.spends_records(), zero);
    BOOST_REQUIRE_EQUAL(query.filter_bk_records(), one);
}

BOOST_AUTO_TEST_CASE(query_extent__input_output_count__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.input_count(0), one);
    BOOST_REQUIRE_EQUAL(query.output_count(0), one);
    BOOST_REQUIRE_EQUAL(query.put_counts(0).first, one);
    BOOST_REQUIRE_EQUAL(query.put_counts(0).second, one);

    BOOST_REQUIRE_EQUAL(query.input_count(1), zero);
    BOOST_REQUIRE_EQUAL(query.output_count(1), zero);
    BOOST_REQUIRE_EQUAL(query.put_counts(1).first, zero);
    BOOST_REQUIRE_EQUAL(query.put_counts(1).second, zero);
}

BOOST_AUTO_TEST_CASE(query_extent__optionals_enabled__default__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.address_enabled());
    BOOST_REQUIRE(query.filter_enabled());
}

BOOST_AUTO_TEST_CASE(query_extent__address_enabled__disabled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.outs.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.address_enabled());
    BOOST_REQUIRE(query.filter_enabled());
}

BOOST_AUTO_TEST_CASE(query_extent__filter_enabled__disabled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.filter_tx.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.address_enabled());
    BOOST_REQUIRE(!query.filter_enabled());
}

// aggregate sizes
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_extent__archive_head_size__genesis__sum_of_tables)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    const auto expected = query.header_head_size()
        + query.output_head_size()
        + query.input_head_size()
        + query.ins_head_size()
        + query.outs_head_size()
        + query.txs_head_size()
        + query.tx_head_size();
    BOOST_REQUIRE_EQUAL(query.archive_head_size(), expected);
}

BOOST_AUTO_TEST_CASE(query_extent__archive_body_size__genesis__sum_of_tables)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    const auto expected = query.header_body_size()
        + query.output_body_size()
        + query.input_body_size()
        + query.ins_body_size()
        + query.outs_body_size()
        + query.txs_body_size()
        + query.tx_body_size();
    BOOST_REQUIRE_EQUAL(query.archive_body_size(), expected);
}

BOOST_AUTO_TEST_CASE(query_extent__archive_size__genesis__head_plus_body)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.archive_size(), query.archive_head_size() + query.archive_body_size());
}

BOOST_AUTO_TEST_CASE(query_extent__store_head_size__genesis__archive_plus_indexes)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    const auto indexes = query.candidate_head_size()
        + query.confirmed_head_size()
        + query.strong_tx_head_size()
        + query.ecdsa_head_size()
        + query.schnorr_head_size()
        + query.silent_head_size()
        + query.duplicate_head_size()
        + query.prevalid_head_size()
        + query.prevout_head_size()
        + query.validated_bk_head_size()
        + query.validated_tx_head_size()
        + query.spends_head_size()
        + query.filter_bk_head_size()
        + query.filter_tx_head_size();
    BOOST_REQUIRE_EQUAL(query.store_head_size(), query.archive_head_size() + indexes);
}

BOOST_AUTO_TEST_CASE(query_extent__store_body_size__genesis__archive_plus_indexes)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    const auto indexes = query.candidate_body_size()
        + query.confirmed_body_size()
        + query.strong_tx_body_size()
        + query.ecdsa_body_size()
        + query.schnorr_body_size()
        + query.silent_body_size()
        + query.duplicate_body_size()
        + query.prevalid_body_size()
        + query.prevout_body_size()
        + query.validated_bk_body_size()
        + query.validated_tx_body_size()
        + query.spends_body_size()
        + query.filter_bk_body_size()
        + query.filter_tx_body_size();
    BOOST_REQUIRE_EQUAL(query.store_body_size(), query.archive_body_size() + indexes);
}

BOOST_AUTO_TEST_CASE(query_extent__store_size__genesis__head_plus_body)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.store_size(), query.store_head_size() + query.store_body_size());
}

BOOST_AUTO_TEST_CASE(query_extent__store_size__archived_block__increases)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    const auto initial = query.store_body_size();
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE_GT(query.store_body_size(), initial);
}

BOOST_AUTO_TEST_SUITE_END()
