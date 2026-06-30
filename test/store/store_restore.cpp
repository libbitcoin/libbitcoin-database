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

BOOST_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// restore
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__restore__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.restore(test::events), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__restore__process_locked__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BOOST_REQUIRE(lock.try_lock());
    BOOST_REQUIRE_EQUAL(instance.restore(test::events), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__restore__no_flush_lock__flush_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.restore(test::events), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__restore__missing_snapshot__missing_snapshot_locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE_EQUAL(instance.restore(test::events), error::missing_snapshot);
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
}

BOOST_AUTO_TEST_CASE(store__restore__success__success_locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(instance.restore(test::events));
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
}

BOOST_AUTO_TEST_CASE(store__restore__no_backups__missing_snapshot_locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::missing_snapshot);
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__restore__primary_open__clear_directory)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));

    // Create /primary directory, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    // Hits the process lock from being open.
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::process_lock);

    ////// Cannot delete /heads with open files.
    ////BOOST_REQUIRE_EQUAL(instance.restore_(), error::clear_directory);
    BOOST_REQUIRE(!instance.close(test::events));

}
#endif

BOOST_AUTO_TEST_CASE(store__restore__primary_closed__open_failure)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE(!instance.create(test::events));

    // Must be closed for restore.
    BOOST_REQUIRE(!instance.close(test::events));

    // Create /primary, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    // There are no backup index files to open.
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(instance.restore_());

    // Rename /primary to /heads and copy to /primary.
    ////BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

BOOST_AUTO_TEST_CASE(store__restore__secondary_closed__open_failure)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE(!instance.create(test::events));

    // Must be closed for restore.
    BOOST_REQUIRE(!instance.close(test::events));

    // Create /secondary, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::secondary));

    // There are no backup index files to open.
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(instance.restore_());

    // No primary, so rename /secondary to /heads.
    BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

BOOST_AUTO_TEST_CASE(store__restore__primary_secondary_loaded__open_failure)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE(!instance.create(test::events));

    // Must be closed for restore.
    BOOST_REQUIRE(!instance.close(test::events));

    // Create /primary from which to restore, and /secondary.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::secondary));

    // There are no backup index files to open.
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(instance.restore_());

    // Rename /primary to /heads and copy to /primary.
    ////BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

// snapshot-restore
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__restore__snapshot__success_unlocks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;

    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(test::events));
    BOOST_REQUIRE(!instance.snapshot(test::events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!instance.close(test::events));
    BOOST_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BOOST_REQUIRE(!instance.restore(test::events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    ////BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::primary));

    // leaves loaded and unlocked
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(test::exists(instance.process_lock_file()));

    BOOST_REQUIRE(!instance.close(test::events));
    BOOST_REQUIRE(!test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
}

BOOST_AUTO_TEST_SUITE_END()
