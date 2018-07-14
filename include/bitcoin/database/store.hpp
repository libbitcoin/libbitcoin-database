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
#include <string>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

class BCD_API store
  : noncopyable
{
public:
    typedef boost::filesystem::path path;

    static const std::string FLUSH_LOCK;
    static const std::string EXCLUSIVE_LOCK;
    static const std::string BLOCK_TABLE;
    static const std::string CANDIDATE_INDEX;
    static const std::string CONFIRMED_INDEX;
    static const std::string TRANSACTION_INDEX;
    static const std::string TRANSACTION_TABLE;
    static const std::string ADDRESS_TABLE;
    static const std::string ADDRESS_ROWS;

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

    /// Start sequence write with optional flush lock.
    virtual bool begin_write() const;

    /// End sequence write with optional flush unlock.
    virtual bool end_write() const;

    /// True if write flushing is enabled.
    virtual bool flush_each_write() const;

    // File names.
    // ------------------------------------------------------------------------

    /// Content store.
    const path block_table;
    const path candidate_index;
    const path confirmed_index;
    const path transaction_index;
    const path transaction_table;

    /// Optional indexes.
    const path address_table;
    const path address_rows;

protected:
    // The implementation must flush all data to disk here.
    virtual bool flush() const = 0;


private:
    const path prefix_;
    const bool with_indexes_;
    const bool flush_each_write_;
    mutable flush_lock flush_lock_;
    mutable interprocess_lock exclusive_lock_;
};

} // namespace database
} // namespace libbitcoin

#endif
