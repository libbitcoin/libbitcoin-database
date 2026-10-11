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

BOOST_FIXTURE_TEST_SUITE(query_properties_tx_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

BOOST_AUTO_TEST_CASE(query_properties_tx__get_wtxid__not_pooled__computed)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_link link{};
    BOOST_REQUIRE(!query.set_code(link, test::tx4));
    BOOST_REQUIRE_EQUAL(query.get_wtxid(link), test::tx4.hash(true));
    BOOST_REQUIRE_NE(query.get_wtxid(link), test::tx4.hash(false));
}

BOOST_AUTO_TEST_CASE(query_properties_tx__get_wtxid__pooled__pool_columns)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_link link{};
    BOOST_REQUIRE(!query.set_code(link, test::tx4));

    // Pool another tx under the link, so the result must come from the pool.
    const transaction source{ test::tx5.to_data(true), true };
    source.inputs_ptr()->front()->metadata.parent_tx = 42;
    BOOST_REQUIRE(query.set_pooled(link, source, context{ bip113, 8, 9 }));
    BOOST_REQUIRE_EQUAL(query.get_wtxid(link), test::tx5.hash(true));
}

BOOST_AUTO_TEST_SUITE_END()
