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
#include "../mocks/map_store.hpp"

// these include the slow tests (mmap)

BOOST_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// open
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__open__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.open(test::events), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__copen__process_locked__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BOOST_REQUIRE(lock.try_lock());
    BOOST_REQUIRE_EQUAL(instance.open(test::events), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__open__flush_locked__flush_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.flush_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.open(test::events), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__open__process_lock_file__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_CASE(store__open__default__unlocks_transactor_only)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    instance.close(test::events);
}

BOOST_AUTO_TEST_CASE(store__open__uncreated__open_failure)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE(instance.open(test::events));
}

BOOST_AUTO_TEST_CASE(store__open__created__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(!instance.close(test::events));
}

BOOST_AUTO_TEST_SUITE_END()
