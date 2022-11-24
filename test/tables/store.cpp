/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

struct store_setup_fixture
{
    DELETE4(store_setup_fixture);
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

class access
  : public store
{
public:
    using path = std::filesystem::path;
    using store::store;

    // backup internals

    code backup_() NOEXCEPT
    {
        return store::backup();
    }

    code dump_(const std::filesystem::path& folder) NOEXCEPT
    {
        return store::dump(folder);
    }

    code restore_() NOEXCEPT
    {
        return store::restore();
    }

    const settings& configuration() const NOEXCEPT
    {
        return configuration_;
    }

    const path& header_head_file() const NOEXCEPT
    {
        return header_head_.file();
    }

    const path& header_body_file() const NOEXCEPT
    {
        return header_body_.file();
    }

    const path& point_head_file() const NOEXCEPT
    {
        return point_head_.file();
    }

    const path& point_body_file() const NOEXCEPT
    {
        return point_body_.file();
    }

    const path& input_head_file() const NOEXCEPT
    {
        return input_head_.file();
    }

    const path& input_body_file() const NOEXCEPT
    {
        return input_body_.file();
    }

    const path& output_body_file() const NOEXCEPT
    {
        return output_body_.file();
    }

    const path& puts_body_file() const NOEXCEPT
    {
        return puts_body_.file();
    }

    const path& tx_head_file() const NOEXCEPT
    {
        return tx_head_.file();
    }

    const path& tx_body_file() const NOEXCEPT
    {
        return tx_body_.file();
    }

    const path& txs_head_file() const NOEXCEPT
    {
        return txs_head_.file();
    }

    const path& txs_body_file() const NOEXCEPT
    {
        return txs_body_.file();
    }

    const path& flush_lock_file() const NOEXCEPT
    {
        return flush_lock_.file();
    }

    const path& process_lock_file() const NOEXCEPT
    {
        return process_lock_.file();
    }

    boost::upgrade_mutex& transactor_mutex() NOEXCEPT
    {
        return transactor_mutex_;
    }
};

// construct
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__construct__default_configuration__referenced)
{
    const settings configuration{};
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(&instance.configuration(), &configuration);
}

BOOST_AUTO_TEST_CASE(store__paths__default_configuration__expected)
{
    const settings configuration{};
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.header_head_file(), "bitcoin/index/archive_header.idx");
    BOOST_REQUIRE_EQUAL(instance.header_body_file(), "bitcoin/archive_header.dat");
    BOOST_REQUIRE_EQUAL(instance.point_head_file(), "bitcoin/index/archive_point.idx");
    BOOST_REQUIRE_EQUAL(instance.point_body_file(), "bitcoin/archive_point.dat");
    BOOST_REQUIRE_EQUAL(instance.input_head_file(), "bitcoin/index/archive_input.idx");
    BOOST_REQUIRE_EQUAL(instance.input_body_file(), "bitcoin/archive_input.dat");
    BOOST_REQUIRE_EQUAL(instance.output_body_file(), "bitcoin/archive_output.dat");
    BOOST_REQUIRE_EQUAL(instance.puts_body_file(), "bitcoin/archive_puts.dat");
    BOOST_REQUIRE_EQUAL(instance.tx_head_file(), "bitcoin/index/archive_tx.idx");
    BOOST_REQUIRE_EQUAL(instance.tx_body_file(), "bitcoin/archive_tx.dat");
    BOOST_REQUIRE_EQUAL(instance.txs_head_file(), "bitcoin/index/archive_txs.idx");
    BOOST_REQUIRE_EQUAL(instance.txs_body_file(), "bitcoin/archive_txs.dat");
    BOOST_REQUIRE_EQUAL(instance.flush_lock_file(), "bitcoin/flush.lock");
    BOOST_REQUIRE_EQUAL(instance.process_lock_file(), "bitcoin/process.lock");
}

// create
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__create__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.create(), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__create__process_locked__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BOOST_REQUIRE(lock.try_lock());
    BOOST_REQUIRE_EQUAL(instance.create(), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__create__flush_locked__flush_lock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::create(instance.flush_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.create(), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__create__process_lock_file__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
}

BOOST_AUTO_TEST_CASE(store__create__default__unlocks)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE(!test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
}

BOOST_AUTO_TEST_CASE(store__create__default__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
}

// create is index-destructive (by directory)
BOOST_AUTO_TEST_CASE(store__create__existing_index__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::indexes));
    BOOST_REQUIRE(test::create(instance.header_head_file()));
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
}

// create is body-destructive (by file)
BOOST_AUTO_TEST_CASE(store__create__existing_body__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::create(instance.header_body_file()));
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
}

// open
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__open__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.open(), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__copen__process_locked__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BOOST_REQUIRE(lock.try_lock());
    BOOST_REQUIRE_EQUAL(instance.open(), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__open__flush_locked__flush_lock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::create(instance.flush_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.open(), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__open__process_lock_file__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    instance.close();
}

BOOST_AUTO_TEST_CASE(store__open__default__unlocks_transactor_only)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE(test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    instance.close();
}

BOOST_AUTO_TEST_CASE(store__open__uncreated__open_failure)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.open(), error::open_failure);
    instance.close();
}

BOOST_AUTO_TEST_CASE(store__open__created__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    instance.close();
}

// snapshot
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__snapshot__uncreated__flush_unloaded)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.snapshot(), error::flush_unloaded);
}

BOOST_AUTO_TEST_CASE(store__snapshot__unopened__flush_unloaded)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.snapshot(), error::flush_unloaded);
}

BOOST_AUTO_TEST_CASE(store__snapshot__opened__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE_EQUAL(instance.snapshot(), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
}

// close
// ----------------------------------------------------------------------------

// flush_unlock is not idempotent
BOOST_AUTO_TEST_CASE(store__close__uncreated__flush_unlock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.close(), error::flush_unlock);
}

// flush_unlock is not idempotent
BOOST_AUTO_TEST_CASE(store__close__unopened__flush_unlock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::flush_unlock);
}

BOOST_AUTO_TEST_CASE(store__close__opened__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    store instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
}

// get_transactor
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__get_transactor__always__share_locked)
{
    const settings configuration{};
    access instance{ configuration };
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
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.backup_(), error::backup_table);
}

BOOST_AUTO_TEST_CASE(store__backup__loaded__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE_EQUAL(instance.backup_(), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(!test::folder(configuration.dir / schema::dir::secondary));
}

BOOST_AUTO_TEST_CASE(store__backup__primary_loaded__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE_EQUAL(instance.backup_(), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::secondary));
}

BOOST_AUTO_TEST_CASE(store__backup__primary_secondary_loaded__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::secondary));
    BOOST_REQUIRE_EQUAL(instance.backup_(), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::secondary));
}

// dump
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__dump__unloaded__unloaded_file)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.dump_(TEST_DIRECTORY), error::unloaded_file);
}

BOOST_AUTO_TEST_CASE(store__dump__loaded__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE_EQUAL(instance.dump_(TEST_DIRECTORY), error::success);
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
}

// restore
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(store__restore__transactor_locked__transactor_lock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    instance.transactor_mutex().lock();
    BOOST_REQUIRE_EQUAL(instance.restore(), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__restore__process_locked__success)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BOOST_REQUIRE(lock.try_lock());
    BOOST_REQUIRE_EQUAL(instance.restore(), error::process_lock);
}
#endif

BOOST_AUTO_TEST_CASE(store__restore__flush_locked__flush_lock)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::create(instance.flush_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.restore(), error::flush_lock);
}

BOOST_AUTO_TEST_CASE(store__restore__process_lock_file__missing_backup)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE(test::create(instance.process_lock_file()));
    BOOST_REQUIRE_EQUAL(instance.restore(), error::missing_backup);
}

BOOST_AUTO_TEST_CASE(store__restore__failure__unlocks)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_NE(instance.restore(), error::success);
    BOOST_REQUIRE(!test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
}

BOOST_AUTO_TEST_CASE(store__restore__no_backups__missing_backup)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::missing_backup);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_MSC)
BOOST_AUTO_TEST_CASE(store__restore__primary_open__clear_directory)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);

    // Create /primary directory, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::primary));

    // Hits the process lock from being open.
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::process_lock);

    ////// Cannot delete /indexes with open files.
    ////BOOST_REQUIRE_EQUAL(instance.restore_(), error::clear_directory);
    instance.close();
}
#endif

BOOST_AUTO_TEST_CASE(store__restore__primary_closed__restore_table)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);

    // Create /primary, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::primary));

    // There are no backup index files to open.
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::restore_table);

    // Rename /primary to /indexes.
    BOOST_REQUIRE(!test::folder(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::indexes));
}

BOOST_AUTO_TEST_CASE(store__restore__secondary_closed__restore_table)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);

    // Create /secondary, from which to restore.
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::secondary));

    // There are no backup index files to open.
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::restore_table);

    // No primary, so rename /secondary to /indexes.
    BOOST_REQUIRE(!test::folder(configuration.dir / schema::dir::secondary));
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::indexes));
}

BOOST_AUTO_TEST_CASE(store__restore__primary_secondary_loaded__restore_table)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };

    // Create /index, to be purged.
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);

    // Create /primary from which to restore, and /secondary.
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(test::clear(configuration.dir / schema::dir::secondary));

    // There are no backup index files to open.
    BOOST_REQUIRE_EQUAL(instance.restore_(), error::restore_table);

    // Rename /primary to /indexes.
    BOOST_REQUIRE(!test::folder(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::secondary));
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::indexes));
}

// backup-restore
// ----------------------------------------------------------------------------


BOOST_AUTO_TEST_CASE(store__restore__snapshot__success_unlocks)
{
    settings configuration{};
    configuration.dir = TEST_DIRECTORY;
    access instance{ configuration };
    BOOST_REQUIRE_EQUAL(instance.create(), error::success);
    BOOST_REQUIRE_EQUAL(instance.open(), error::success);
    BOOST_REQUIRE_EQUAL(instance.snapshot(), error::success);
    BOOST_REQUIRE(test::folder(configuration.dir / schema::dir::primary));
    BOOST_REQUIRE_EQUAL(instance.close(), error::success);
    BOOST_REQUIRE_EQUAL(instance.restore(), error::success);
    BOOST_REQUIRE(!test::folder(configuration.dir / schema::dir::primary));

    BOOST_REQUIRE(!test::exists(instance.flush_lock_file()));
    BOOST_REQUIRE(!test::exists(instance.process_lock_file()));
    BOOST_REQUIRE(instance.transactor_mutex().try_lock());
}

BOOST_AUTO_TEST_SUITE_END()
