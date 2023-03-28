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

BOOST_AUTO_TEST_CASE(query_extent__size__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.header_size(), schema::header::minrow);
    BOOST_REQUIRE_EQUAL(query.output_size(), 82u);
    BOOST_REQUIRE_EQUAL(query.input_size(), 100u);
    BOOST_REQUIRE_EQUAL(query.point_size(), 0u); 
    BOOST_REQUIRE_EQUAL(query.puts_size(), 2 * schema::put);
    BOOST_REQUIRE_EQUAL(query.txs_size(), schema::txs::minrow + 2 * schema::tx);
    BOOST_REQUIRE_EQUAL(query.tx_size(), schema::transaction::minrow);
    BOOST_REQUIRE_EQUAL(query.archive_size(), schema::header::minrow + 82u + 100u + 0u + 2 * schema::put + schema::txs::minrow + 2 * schema::tx + schema::transaction::minrow);
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
    BOOST_REQUIRE_EQUAL(query.input_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.txs_buckets(), 100u);
    BOOST_REQUIRE_EQUAL(query.tx_buckets(), 100u);
}

BOOST_AUTO_TEST_CASE(query_extent__records__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    // point_records is zero because there are no spends in genesis.
    BOOST_REQUIRE_EQUAL(query.header_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.point_records(), 0u);
    BOOST_REQUIRE_EQUAL(query.puts_records(), 2u);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 1u);
}

BOOST_AUTO_TEST_CASE(query_extent__slabs__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.input_slabs(1), zero);
    BOOST_REQUIRE_EQUAL(query.output_slabs(1), zero);
    BOOST_REQUIRE_EQUAL(query.put_slabs(1).first, zero);
    BOOST_REQUIRE_EQUAL(query.put_slabs(1).second, zero);
}

BOOST_AUTO_TEST_SUITE_END()
