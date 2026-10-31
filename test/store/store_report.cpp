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
#include "../mocks/map_store.hpp"

BC_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// report
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__report__created__reports_all_tables_without_fault)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    size_t calls{};
    size_t faults{};
    instance.report([&](const code& ec, auto) NOEXCEPT { ++calls; faults += static_cast<size_t>(static_cast<bool>(ec)); });
    BC_REQUIRE_NE(calls, 0u);
    BC_REQUIRE_EQUAL(faults, 0u);
    BC_REQUIRE(!instance.close(test::events));
}

// get_fault
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__get_fault__created__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE(!instance.close(test::events));
}

// get_space
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__get_space__created__zero)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE_EQUAL(instance.get_space(), 0u);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_SUITE_END()
