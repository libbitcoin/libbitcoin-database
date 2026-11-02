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

// create
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__create__transactor_locked__transactor_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BC_REQUIRE_EQUAL(instance.create(test::events), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_WINDOWS)
BC_AUTO_TEST_CASE(store__create__process_locked__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BC_REQUIRE(lock.try_lock());
    BC_REQUIRE_EQUAL(instance.create(test::events), error::process_lock);
}
#endif

BC_AUTO_TEST_CASE(store__create__flush_locked__flush_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.flush_lock_file()));
    BC_REQUIRE_EQUAL(instance.create(test::events), error::flush_lock);
}

BC_AUTO_TEST_CASE(store__create__process_lock_file__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.process_lock_file()));
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__create__default__locks)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(test::exists(instance.flush_lock_file()));
    BC_REQUIRE(test::exists(instance.process_lock_file()));
    BC_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__create__default__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

// create is index-destructive (by directory)
BC_AUTO_TEST_CASE(store__create__existing_index__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::clear(configuration.path / schema::dir::heads));
    BC_REQUIRE(test::create(instance.header_head_file()));
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

// create is body-destructive (by file)
BC_AUTO_TEST_CASE(store__create__existing_body__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.header_body_file()));
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_SUITE_END()
