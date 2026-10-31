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

BC_FIXTURE_TEST_SUITE(query_sizes_tests, test::directory_setup_fixture)

// get_tx_sizes

BC_AUTO_TEST_CASE(query_sizes__get_tx_sizes__coinbase__204)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_CHECK(!store.create(test::events_handler));
    BC_CHECK(query.initialize(test::genesis));

    size_t light{};
    size_t heavy{};
    BC_CHECK(query.get_tx_sizes(light, heavy, 0));
    BC_CHECK_EQUAL(light, 204u);
    BC_CHECK_EQUAL(heavy, 204u);
}

BC_AUTO_TEST_SUITE_END()
