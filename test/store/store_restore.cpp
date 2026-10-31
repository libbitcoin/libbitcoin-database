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

// restore
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__restore__transactor_locked__transactor_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BC_REQUIRE_EQUAL(instance.restore(test::events), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_WINDOWS)
BC_AUTO_TEST_CASE(store__restore__process_locked__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BC_REQUIRE(lock.try_lock());
    BC_REQUIRE_EQUAL(instance.restore(test::events), error::process_lock);
}
#endif

BC_AUTO_TEST_CASE(store__restore__no_flush_lock__flush_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE_EQUAL(instance.restore(test::events), error::flush_lock);
}

BC_AUTO_TEST_CASE(store__restore__missing_snapshot__recreate_success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.process_lock_file()));
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE(test::exists(instance.flush_lock_file()));
    BC_REQUIRE(test::exists(instance.process_lock_file()));
    BC_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__restore__no_backups__recreate_success_locks)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore_());
    BC_REQUIRE(test::exists(instance.flush_lock_file()));
    BC_REQUIRE(test::exists(instance.process_lock_file()));
    BC_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
    BC_REQUIRE(!test::exists(instance.process_lock_file()));
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_WINDOWS)
BC_AUTO_TEST_CASE(store__restore__primary_open__clear_directory)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));

    // Create /primary directory, from which to restore.
    BC_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    // Hits the process lock from being open.
    BC_REQUIRE_EQUAL(instance.restore_(), error::process_lock);

    ////// Cannot delete /heads with open files.
    ////BC_REQUIRE_EQUAL(instance.restore_(), error::clear_directory);
    BC_REQUIRE(!instance.close(test::events));

}
#endif

BC_AUTO_TEST_CASE(store__restore__primary_closed__open_failure)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BC_REQUIRE(!instance.create(test::events));

    // Must be closed for restore.
    BC_REQUIRE(!instance.close(test::events));

    // Create /primary, from which to restore.
    BC_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    // There are no backup index files to open.
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(instance.restore_());

    // Rename /primary to /heads and copy to /primary.
    ////BC_REQUIRE(!test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

BC_AUTO_TEST_CASE(store__restore__secondary_closed__open_failure)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BC_REQUIRE(!instance.create(test::events));

    // Must be closed for restore.
    BC_REQUIRE(!instance.close(test::events));

    // Create /secondary, from which to restore.
    BC_REQUIRE(test::clear(configuration.path / schema::dir::secondary));

    // There are no backup index files to open.
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(instance.restore_());

    // No primary, so rename /secondary to /heads.
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

BC_AUTO_TEST_CASE(store__restore__primary_secondary_loaded__open_failure)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BC_REQUIRE(!instance.create(test::events));

    // Must be closed for restore.
    BC_REQUIRE(!instance.close(test::events));

    // Create /primary from which to restore, and /secondary.
    BC_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::secondary));

    // There are no backup index files to open.
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(instance.restore_());

    // Rename /primary to /heads and copy to /primary.
    ////BC_REQUIRE(!test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

// snapshot-restore
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__restore__snapshot__success_unlocks)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;

    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.snapshot(test::events));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    ////BC_REQUIRE(!test::folder(configuration.path / schema::dir::primary));

    // leaves loaded and unlocked
    BC_REQUIRE(test::exists(instance.flush_lock_file()));
    BC_REQUIRE(test::exists(instance.process_lock_file()));

    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
    BC_REQUIRE(!test::exists(instance.process_lock_file()));
}

BC_AUTO_TEST_CASE(store__restore__secondary_remnant__deleted)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    const auto primary = configuration.path / schema::dir::primary;
    const auto secondary = configuration.path / schema::dir::secondary;

    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.snapshot(test::events));
    BC_REQUIRE(!instance.close(test::events));

    // Simulate crash between backup promotion and /secondary deletion.
    BC_REQUIRE(file::copy_directory(primary, secondary));
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE(test::folder(primary));
    BC_REQUIRE(!test::folder(secondary));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__restore__secondary_only__promoted_success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    const auto primary = configuration.path / schema::dir::primary;
    const auto secondary = configuration.path / schema::dir::secondary;

    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.snapshot(test::events));
    BC_REQUIRE(!instance.close(test::events));

    // Simulate crash between backup rotation and /primary promotion.
    BC_REQUIRE(file::rename(primary, secondary));
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE(test::folder(primary));
    BC_REQUIRE(!test::folder(secondary));
    BC_REQUIRE(test::folder(configuration.path / schema::dir::heads));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_SUITE_END()
