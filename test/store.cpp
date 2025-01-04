/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include "test.hpp"
#include "mocks/map_store.hpp"

 // these are the slow tests (mmap)

struct store_setup_fixture
{
    DELETE_COPY_MOVE(store_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    store_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~store_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(store_tests, store_setup_fixture)

// nop event handler.
const auto events = [](auto, auto){};

// flush lock file path from directory.
std::filesystem::path flush_lock_file(std::filesystem::path path)
{
    path /= schema::locks::flush;
    path += schema::ext::lock;
    return path;
}

// construct
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__construct__default_configuration__referenced)
{
    const settings configuration{};
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(&instance.configuration(), &configuration);
}

BOOST_AUTO_TEST_CASE(store__paths__default_configuration__expected)
{
    const settings configuration{};
    test::map_store instance{ configuration };

    /// Archive.
    BOOST_REQUIRE_EQUAL(instance.header_head_file(), "bitcoin/heads/archive_header.head");
    BOOST_REQUIRE_EQUAL(instance.header_body_file(), "bitcoin/archive_header.data");
    BOOST_REQUIRE_EQUAL(instance.point_head_file(), "bitcoin/heads/archive_point.head");
    BOOST_REQUIRE_EQUAL(instance.point_body_file(), "bitcoin/archive_point.data");
    BOOST_REQUIRE_EQUAL(instance.input_head_file(), "bitcoin/heads/archive_input.head");
    BOOST_REQUIRE_EQUAL(instance.input_body_file(), "bitcoin/archive_input.data");
    BOOST_REQUIRE_EQUAL(instance.output_head_file(), "bitcoin/heads/archive_output.head");
    BOOST_REQUIRE_EQUAL(instance.output_body_file(), "bitcoin/archive_output.data");
    BOOST_REQUIRE_EQUAL(instance.puts_head_file(), "bitcoin/heads/archive_puts.head");
    BOOST_REQUIRE_EQUAL(instance.puts_body_file(), "bitcoin/archive_puts.data");
    BOOST_REQUIRE_EQUAL(instance.tx_head_file(), "bitcoin/heads/archive_tx.head");
    BOOST_REQUIRE_EQUAL(instance.tx_body_file(), "bitcoin/archive_tx.data");
    BOOST_REQUIRE_EQUAL(instance.txs_head_file(), "bitcoin/heads/archive_txs.head");
    BOOST_REQUIRE_EQUAL(instance.txs_body_file(), "bitcoin/archive_txs.data");

    /// Index.
    BOOST_REQUIRE_EQUAL(instance.address_head_file(), "bitcoin/heads/address.head");
    BOOST_REQUIRE_EQUAL(instance.address_body_file(), "bitcoin/address.data");
    BOOST_REQUIRE_EQUAL(instance.candidate_head_file(), "bitcoin/heads/candidate.head");
    BOOST_REQUIRE_EQUAL(instance.candidate_body_file(), "bitcoin/candidate.data");
    BOOST_REQUIRE_EQUAL(instance.confirmed_head_file(), "bitcoin/heads/confirmed.head");
    BOOST_REQUIRE_EQUAL(instance.confirmed_body_file(), "bitcoin/confirmed.data");
    BOOST_REQUIRE_EQUAL(instance.spend_head_file(), "bitcoin/heads/archive_spend.head");
    BOOST_REQUIRE_EQUAL(instance.spend_body_file(), "bitcoin/archive_spend.data");
    BOOST_REQUIRE_EQUAL(instance.strong_tx_head_file(), "bitcoin/heads/strong_tx.head");
    BOOST_REQUIRE_EQUAL(instance.strong_tx_body_file(), "bitcoin/strong_tx.data");

    /// Caches.
    BOOST_REQUIRE_EQUAL(instance.validated_bk_head_file(), "bitcoin/heads/validated_bk.head");
    BOOST_REQUIRE_EQUAL(instance.validated_bk_body_file(), "bitcoin/validated_bk.data");
    BOOST_REQUIRE_EQUAL(instance.validated_tx_head_file(), "bitcoin/heads/validated_tx.head");
    BOOST_REQUIRE_EQUAL(instance.validated_tx_body_file(), "bitcoin/validated_tx.data");
    BOOST_REQUIRE_EQUAL(instance.neutrino_head_file(), "bitcoin/heads/neutrino.head");
    BOOST_REQUIRE_EQUAL(instance.neutrino_body_file(), "bitcoin/neutrino.data");
    ////BOOST_REQUIRE_EQUAL(instance.bootstrap_head_file(), "bitcoin/heads/bootstrap.head");
    ////BOOST_REQUIRE_EQUAL(instance.bootstrap_body_file(), "bitcoin/bootstrap.data");
    ////BOOST_REQUIRE_EQUAL(instance.buffer_head_file(), "bitcoin/heads/buffer.head");
    ////BOOST_REQUIRE_EQUAL(instance.buffer_body_file(), "bitcoin/buffer.data");

    /// Locks.
    BOOST_REQUIRE_EQUAL(instance.flush_lock_file(), "bitcoin/flush.lock");
    BOOST_REQUIRE_EQUAL(instance.process_lock_file(), "bitcoin/process.lock");
}

// create
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__create__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.create(events), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__create__process_locked__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BOOST_REQUIRE(lock.try_lock());
    BOOST_REQUIRE_EQUAL(instance.create(events), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__create__flush_locked__flush_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.flush_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.create(events), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__create__process_lock_file__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

BOOST_AUTO_TEST_CASE(store__create__default__locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    BOOST_REQUIRE(!instance.close(events));
}

BOOST_AUTO_TEST_CASE(store__create__default__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

// create is index-destructive (by directory)
BOOST_AUTO_TEST_CASE(store__create__existing_index__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::heads));
    BOOST_REQUIRE(test::create(instance.header_head_file()));
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

// create is body-destructive (by file)
BOOST_AUTO_TEST_CASE(store__create__existing_body__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.header_body_file()));
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

// open
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__open__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.open(events), error::transactor_lock);
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
    BOOST_REQUIRE_EQUAL(instance.open(events), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__open__flush_locked__flush_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.flush_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.open(events), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__open__process_lock_file__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

BOOST_AUTO_TEST_CASE(store__open__default__unlocks_transactor_only)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    instance.close(events);
}

BOOST_AUTO_TEST_CASE(store__open__uncreated__open_failure)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE(instance.open(events));
}

BOOST_AUTO_TEST_CASE(store__open__created__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

// snapshot
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__snapshot__uncreated__flush_unloaded)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.snapshot(events), error::flush_unloaded);
}

BOOST_AUTO_TEST_CASE(store__snapshot__opened__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.snapshot(events));
    BOOST_REQUIRE(!instance.close(events));
}

// close
// ----------------------------------------------------------------------------

// flush_unlock is not idempotent
BOOST_AUTO_TEST_CASE(store__close__uncreated__flush_unlock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.close(events), error::flush_unlock);
}

BOOST_AUTO_TEST_CASE(store__close__opened__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    store<map> instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.close(events));
}

// get_transactor
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__get_transactor__always__share_locked)
{
    const settings configuration{};
    test::map_store instance{ configuration };
    auto transactor = instance.get_transactor();
    BOOST_REQUIRE(transactor);
    BOOST_REQUIRE(!instance.transactor_mutex().try_lock());
    BOOST_REQUIRE(instance.transactor_mutex().try_lock_shared());
}

// backup
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__backup__unloaded__backup_table)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.backup_(), error::backup_table);
}

BOOST_AUTO_TEST_CASE(store__backup__loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.backup_());
    BOOST_REQUIRE(!instance.close(events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
}

BOOST_AUTO_TEST_CASE(store__backup__primary_loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!instance.backup_());
    BOOST_REQUIRE(!instance.close(events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
}

BOOST_AUTO_TEST_CASE(store__backup__primary_secondary_loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::secondary));
    BOOST_REQUIRE(!instance.backup_());
    BOOST_REQUIRE(!instance.close(events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
}

// dump
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__dump__unloaded__unloaded_file)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.dump_(TEST_DIRECTORY), error::unloaded_file);
}

BOOST_AUTO_TEST_CASE(store__dump__loaded__success)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.dump_(TEST_DIRECTORY));
    BOOST_REQUIRE(!instance.close(events));
}

// restore
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__restore__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.restore(events), error::transactor_lock);
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
    BOOST_REQUIRE_EQUAL(instance.restore(events), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__restore__no_flush_lock__flush_lock)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.restore(events), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__restore__missing_snapshot__missing_snapshot_locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
    BOOST_REQUIRE_EQUAL(instance.restore(events), error::missing_snapshot);
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
}

BOOST_AUTO_TEST_CASE(store__restore__success__success_locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
    BOOST_REQUIRE(instance.restore(events));
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
}

BOOST_AUTO_TEST_CASE(store__restore__no_backups__missing_snapshot_locks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
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
    BOOST_REQUIRE(!instance.create(events));

    // Create /primary directory, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    // Hits the process lock from being open.
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::process_lock);

    ////// Cannot delete /heads with open files.
    ////BOOST_REQUIRE_EQUAL(instance.restore_(), error::clear_directory);
    BOOST_REQUIRE(!instance.close(events));

}
#endif

BOOST_AUTO_TEST_CASE(store__restore__primary_closed__open_failure)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE(!instance.create(events));

    // Must be closed for restore.
    BOOST_REQUIRE(!instance.close(events));

    // Create /primary, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    // There are no backup index files to open.
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
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
    BOOST_REQUIRE(!instance.create(events));

    // Must be closed for restore.
    BOOST_REQUIRE(!instance.close(events));

    // Create /secondary, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::secondary));

    // There are no backup index files to open.
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
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
    BOOST_REQUIRE(!instance.create(events));

    // Must be closed for restore.
    BOOST_REQUIRE(!instance.close(events));

    // Create /primary from which to restore, and /secondary.
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::clear(configuration.path / schema::dir::secondary));

    // There are no backup index files to open.
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
    BOOST_REQUIRE(instance.restore_());

    // Rename /primary to /heads and copy to /primary.
    ////BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::secondary));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::heads));
}

// backup-restore
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__restore__snapshot__success_unlocks)
{
    settings configuration{};
    configuration.path = TEST_DIRECTORY;

    test::map_store instance{ configuration };
    BOOST_REQUIRE(!instance.create(events));
    BOOST_REQUIRE(!instance.snapshot(events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BOOST_REQUIRE(!instance.close(events));
    BOOST_REQUIRE(test::create(flush_lock_file(configuration.path)));
    BOOST_REQUIRE(!instance.restore(events));
    BOOST_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    ////BOOST_REQUIRE(!test::folder(configuration.path / schema::dir::primary));

    // leaves loaded and unlocked
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(test::exists(instance.process_lock_file()));

    BOOST_REQUIRE(!instance.close(events));
    BOOST_REQUIRE(!test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
}

BOOST_AUTO_TEST_SUITE_END()
