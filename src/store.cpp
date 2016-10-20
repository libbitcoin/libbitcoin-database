/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/store.hpp>

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::database;
using path = boost::filesystem::path;

// The sentinel max_uint32 is used to align with fixed-width config settings,
// and size_t is used to align with the database height domain.
const size_t store::without_indexes = max_uint32;

// static
bool store::create(const path& file_path)
{
    bc::ofstream file(file_path.string());

    if (file.bad())
        return false;

    // Write one byte so file is nonzero size.
    file.put('x');
    return true;
}

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes)
  : with_indexes_(with_indexes),
    sequential_lock_(0),
    lock_file_(prefix / "process_lock"),

    // Content store.
    block_table(prefix / "block_table"),
    block_index(prefix / "block_index"),
    transaction_table(prefix / "transaction_table"),

    // Optional indexes.
    spend_table(prefix / "spend_table"),
    history_table(prefix / "history_table"),
    history_rows(prefix / "history_rows"),
    stealth_rows(prefix / "stealth_rows")
{
}

// Open and close.
// ------------------------------------------------------------------------

// Create files.
bool store::create() const
{
    const auto created_content =
        create(block_table) &&
        create(block_index) &&
        create(transaction_table);

    const auto create_indexes = [this]()
    {
        return
            create(spend_table) &&
            create(history_table) &&
            create(history_rows) &&
            create(stealth_rows);
    };

    return created_content && (!with_indexes_ || create_indexes());
}

// Lock file access.
bool store::open()
{
    // Create the file lock resource (file).
    if (!create(lock_file_))
        return false;

    // Create the file lock instance.
    process_lock_ = std::make_shared<file_lock>(lock_file_.string());

    // Attempt to acquire the lock (without waiting).
    // If no other process has exclusive, or sharable ownership this succeeds.
    return process_lock_->try_lock();
}

// Unlock file access.
bool store::close()
{
    // This may leave the lock file behind, which is not a problem.
    if (!process_lock_)
        return false;

    process_lock_.reset();
    return boost::filesystem::remove(lock_file_);
}

// Sequential locking.
// ----------------------------------------------------------------------------

store::handle store::begin_read() const
{
    return sequential_lock_.load();
}

bool store::is_read_valid(handle value) const
{
    return value == sequential_lock_.load();
}

// TODO: drop a file as a write sentinel that we can use to detect uncontrolled
// shutdown during write. Use a similar approach around initial block download.
// Fail startup if the sentinel is detected. (file: write_lock).
bool store::begin_write()
{
    ///////////////////////////////////////////////////////////////////////////
    // slock is now odd.
    return is_write_locked(++sequential_lock_);
}

// TODO: clear the write sentinel.
bool store::end_write()
{
    // slock_ is now even again.
    return !is_write_locked(++sequential_lock_);
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace data_base
} // namespace libbitcoin
