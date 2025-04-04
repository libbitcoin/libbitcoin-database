/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

struct query_extent_setup_fixture
{
    DELETE_COPY_MOVE(query_extent_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_extent_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_extent_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_extent_tests, query_extent_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_body_size(), schema::header::minrow);
    BOOST_REQUIRE_EQUAL(query.output_body_size(), 81u);
    BOOST_REQUIRE_EQUAL(query.input_body_size(), 79u);
    BOOST_REQUIRE_EQUAL(query.point_body_size(), schema::point::minrow);
    BOOST_REQUIRE_EQUAL(query.ins_body_size(), schema::ins::minrow);
    BOOST_REQUIRE_EQUAL(query.outs_body_size(), schema::outs::minrow);
    BOOST_REQUIRE_EQUAL(query.txs_body_size(), schema::txs::minrow);
    BOOST_REQUIRE_EQUAL(query.tx_body_size(), schema::transaction::minrow);

    BOOST_REQUIRE_EQUAL(query.candidate_body_size(), schema::height::minrow);
    BOOST_REQUIRE_EQUAL(query.confirmed_body_size(), schema::height::minrow);
    BOOST_REQUIRE_EQUAL(query.strong_tx_body_size(), schema::strong_tx::minrow);
    BOOST_REQUIRE_EQUAL(query.duplicate_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.prevout_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.validated_bk_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.validated_tx_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.filter_bk_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.filter_tx_body_size(), zero);
    BOOST_REQUIRE_EQUAL(query.address_body_size(), schema::address::minrow);
}

BOOST_AUTO_TEST_CASE(query_extent__buckets__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.point_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.txs_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.tx_buckets(), 128u);

    BOOST_REQUIRE_EQUAL(query.strong_tx_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.duplicate_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.prevout_buckets(), 128);
    BOOST_REQUIRE_EQUAL(query.validated_tx_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.validated_bk_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.filter_tx_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.filter_bk_buckets(), 128u);
    BOOST_REQUIRE_EQUAL(query.address_buckets(), 128u);
}

BOOST_AUTO_TEST_CASE(query_extent__records__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_records(), one);
    BOOST_REQUIRE_EQUAL(query.point_records(), one);
    BOOST_REQUIRE_EQUAL(query.ins_records(), one);
    BOOST_REQUIRE_EQUAL(query.tx_records(), one);

    BOOST_REQUIRE_EQUAL(query.candidate_records(), one);
    BOOST_REQUIRE_EQUAL(query.confirmed_records(), one);
    BOOST_REQUIRE_EQUAL(query.strong_tx_records(), one);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), zero);
    BOOST_REQUIRE_EQUAL(query.filter_bk_records(), zero);
    BOOST_REQUIRE_EQUAL(query.address_records(), one);
}

BOOST_AUTO_TEST_CASE(query_extent__input_output_count__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.address_enabled());
    BOOST_REQUIRE(query.filter_enabled());
}

BOOST_AUTO_TEST_CASE(query_extent__address_enabled__disabled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.address_buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.address_enabled());
    BOOST_REQUIRE(query.filter_enabled());
}

BOOST_AUTO_TEST_CASE(query_extent__filter_enabled__disabled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.filter_tx_buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.address_enabled());
    BOOST_REQUIRE(!query.filter_enabled());
}

BOOST_AUTO_TEST_SUITE_END()
