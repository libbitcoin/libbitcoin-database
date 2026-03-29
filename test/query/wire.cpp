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
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_wire_tests, test::directory_setup_fixture)

// get_wire_header1

BOOST_AUTO_TEST_CASE(query_wire__get_wire_header__genesis__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_header(0), to_chunk(test::header0_data));
    BOOST_CHECK(!store.close(test::events_handler));
}

// get_wire_header2
// get_wire_tx1
// get_wire_tx2
// get_wire_block2

BOOST_AUTO_TEST_SUITE_END()
