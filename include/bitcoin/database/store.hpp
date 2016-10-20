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
#ifndef LIBBITCOIN_DATABASE_STORE_HPP
#define LIBBITCOIN_DATABASE_STORE_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/unicode/file_lock.hpp>

namespace libbitcoin {
namespace database {

class BCD_API store
{
public:
    typedef uint64_t handle;
    typedef boost::filesystem::path path;

    static const size_t without_indexes;

    /// Create a single file with one byte of arbitrary data.
    static bool create(const path& file_path);

    /// Delete the file with the given path.
    static bool destroy(const path& file_path);

    /// Determine if the given handle is a write-locked handle.
    static inline bool is_write_locked(handle value)
    {
        return (value % 2) == 1;
    }

    // Construct.
    // ------------------------------------------------------------------------

    store(const path& prefix, bool with_indexes);

    // Open and close.
    // ------------------------------------------------------------------------

    /// Create database files.
    virtual bool create() const;

    /// Acquire exclusive access.
    virtual bool open();

    /// Release exclusive access.
    virtual bool close();

    // Sequential locking.
    // ------------------------------------------------------------------------

    handle begin_read() const;
    bool is_read_valid(handle handle) const;

    bool begin_write();
    bool end_write();

    // File names.
    // ------------------------------------------------------------------------

    // Content store.
    const path block_table;
    const path block_index;
    const path transaction_table;

    // Optional indexes.
    const path spend_table;
    const path history_table;
    const path history_rows;
    const path stealth_rows;

protected:

    // Create spend, history and stealth indexes.
    const bool with_indexes_;

private:
    typedef std::atomic<size_t> sequential_lock;
    typedef std::shared_ptr<database::file_lock> process_lock;

    // Atomic counter for implementing the sequential lock pattern.
    sequential_lock sequential_lock_;

    // Used to restrict database access to this process.
    const path lock_file_;
    process_lock process_lock_;
};

} // namespace database
} // namespace libbitcoin

#endif
