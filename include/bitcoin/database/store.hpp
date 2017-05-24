/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_STORE_HPP
#define LIBBITCOIN_DATABASE_STORE_HPP

#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

class BCD_API store
{
public:
    typedef boost::filesystem::path path;
    typedef sequential_lock::handle handle;

    static const size_t without_indexes;

    /// Create a single file with one byte of arbitrary data.
    static bool create(const path& file_path);

    // Construct.
    // ------------------------------------------------------------------------

    store(const path& prefix, bool with_indexes, bool flush_each_write=false);

    // Open and close.
    // ------------------------------------------------------------------------

    /// Create database files.
    virtual bool create();

    /// Acquire exclusive access.
    virtual bool open();

    /// Release exclusive access.
    virtual bool close();

    // Write with flush detection.
    // ------------------------------------------------------------------------

    /// Start a read sequence and obtain its handle.
    handle begin_read() const;

    /// Check read sequence result of the handle.
    bool is_read_valid(handle handle) const;

    /// Check the write state of the handle.
    bool is_write_locked(handle handle) const;

    /// Start sequence write with optional flush lock.
    bool begin_write() const;

    /// End sequence write with optional flush unlock.
    bool end_write() const;

    /// Optionally begin flush lock scope.
    bool flush_lock() const;

    /// Optionally end flush lock scope.
    bool flush_unlock() const;

    // File names.
    // ------------------------------------------------------------------------

    /// Content store.
    const path header_index;
    const path block_index;
    const path block_table;
    const path transaction_table;
    const path transaction_index;

    /// Optional indexes.
    const path history_rows;
    const path history_table;
    const path spend_table;
    const path stealth_rows;

protected:
    virtual bool flush() const = 0;

    const bool use_indexes;

private:
    const bool flush_each_write_;
    mutable bc::flush_lock flush_lock_;
    mutable interprocess_lock exclusive_lock_;
    mutable sequential_lock sequential_lock_;
};

} // namespace database
} // namespace libbitcoin

#endif
