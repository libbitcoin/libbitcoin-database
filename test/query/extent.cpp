/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
const auto events = [](auto, auto) {};

BOOST_AUTO_TEST_CASE(query_extent__sizes__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.archive_size(),
        schema::header::minrow +
        81u + 79u + zero +
        schema::spend::minrow +
        schema::puts::minrow +
        schema::txs::minrow +
        schema::transaction::minrow);
    BOOST_REQUIRE_EQUAL(query.header_size(), schema::header::minrow);
    BOOST_REQUIRE_EQUAL(query.output_size(), 81u);
    BOOST_REQUIRE_EQUAL(query.input_size(), 79u);
    BOOST_REQUIRE_EQUAL(query.point_size(), zero);
    BOOST_REQUIRE_EQUAL(query.spend_size(), schema::spend::minrow);
    BOOST_REQUIRE_EQUAL(query.puts_size(), schema::puts::minrow);
    BOOST_REQUIRE_EQUAL(query.txs_size(), schema::txs::minrow);
    BOOST_REQUIRE_EQUAL(query.tx_size(), schema::transaction::minrow);

    BOOST_REQUIRE_EQUAL(query.candidate_size(), schema::height::minrow);
    BOOST_REQUIRE_EQUAL(query.confirmed_size(), schema::height::minrow);
    BOOST_REQUIRE_EQUAL(query.strong_tx_size(), schema::strong_tx::minrow);
    BOOST_REQUIRE_EQUAL(query.validated_tx_size(), schema::validated_tx::minrow);
    BOOST_REQUIRE_EQUAL(query.validated_bk_size(), schema::validated_bk::minrow);

    BOOST_REQUIRE_EQUAL(query.address_size(), schema::address::minrow);
    BOOST_REQUIRE_EQUAL(query.neutrino_size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_extent__buckets__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.point_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.spend_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.txs_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.tx_buckets(), 100u);

    BOOST_REQUIRE_EQUAL(query.strong_tx_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.validated_tx_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.validated_bk_buckets(), 100u);

    BOOST_REQUIRE_EQUAL(query.address_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.neutrino_buckets(), 100u);
}

BOOST_AUTO_TEST_CASE(query_extent__records__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    // point_records is zero because there is only coinbase in genesis.
    BOOST_REQUIRE_EQUAL(query.header_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.point_records(), 0u);
    BOOST_REQUIRE_EQUAL(query.spend_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 1u);

    BOOST_REQUIRE_EQUAL(query.candidate_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.confirmed_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.strong_tx_records(), 1u);

    BOOST_REQUIRE_EQUAL(query.address_records(), 1u);
}

BOOST_AUTO_TEST_CASE(query_extent__input_output_count__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.input_count(0), 1u);
    BOOST_REQUIRE_EQUAL(query.output_count(0), 1u);
    BOOST_REQUIRE_EQUAL(query.put_counts(0).first, 1u);
    BOOST_REQUIRE_EQUAL(query.put_counts(0).second, 1u);

    BOOST_REQUIRE_EQUAL(query.input_count(1), 0u);
    BOOST_REQUIRE_EQUAL(query.output_count(1), 0u);
    BOOST_REQUIRE_EQUAL(query.put_counts(1).first, 0u);
    BOOST_REQUIRE_EQUAL(query.put_counts(1).second, 0u);
}

BOOST_AUTO_TEST_CASE(query_extent__optionals_enabled__default__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.address_enabled());
    BOOST_REQUIRE(query.neutrino_enabled());
}

BOOST_AUTO_TEST_CASE(query_extent__address_enabled__disabled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.address_buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.address_enabled());
    BOOST_REQUIRE(query.neutrino_enabled());
}

BOOST_AUTO_TEST_CASE(query_extent__neutrino_enabled__disabled__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.neutrino_buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.address_enabled());
    BOOST_REQUIRE(!query.neutrino_enabled());
}

BOOST_AUTO_TEST_SUITE_END()
