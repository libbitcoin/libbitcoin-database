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
#ifndef LIBBITCOIN_DATABASE_HISTORY_DATABASE_HPP
#define LIBBITCOIN_DATABASE_HISTORY_DATABASE_HPP

#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_map.hpp>
#include <bitcoin/database/primitives/record_multimap.hpp>

namespace libbitcoin {
namespace database {

struct BCD_API history_statinfo
{
    /// Number of buckets used in the hashtable.
    /// load factor = addrs / buckets
    const size_t buckets;

    /// Total number of unique addresses in the database.
    const size_t addresses;

    /// Total number of rows across all addresses.
    const size_t rows;
};

/// This is a multimap where the key is the Bitcoin address hash,
/// which returns several rows giving the history for that address.
class BCD_API history_database
{
public:
    typedef boost::filesystem::path path;
    typedef chain::payment_record::list list;

    /// Construct the database.
    history_database(const path& lookup_filename, const path& rows_filename,
        size_t buckets, size_t expansion);

    /// Close the database (all threads must first be stopped).
    ~history_database();

    // Startup and shutdown.
    // ----------------------------------------------------------------------------

    /// Initialize a new history database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Commit latest inserts.
    void commit();

    /// Flush the memory maps to disk.
    bool flush() const;

    /// Call to unload the memory map.
    bool close();

    // Queries.
    //-------------------------------------------------------------------------

    /// Get the output and input points associated with the address hash.
    list get(const short_hash& key, size_t limit, size_t from_height) const;

    /// Return statistical info about the database.
    history_statinfo statinfo() const;

    // Store.
    //-------------------------------------------------------------------------

    /// Add a row for the key. If key doesn't exist it will be created.
    void store(const short_hash& key, const chain::payment_record& payment);

    // Update.
    //-------------------------------------------------------------------------

    /// Logically delete the last row that was added to key.
    bool unlink_last_row(const short_hash& key);

private:
    typedef record_hash_table<short_hash> record_map;
    typedef record_multimap<short_hash> record_multiple_map;

    /// Hash table used for start index lookup for linked list by address hash.
    file_map lookup_file_;
    record_map::header_type lookup_header_;
    record_manager lookup_manager_;
    record_map lookup_map_;

    /// History rows.
    file_map rows_file_;
    record_manager rows_manager_;
    record_multiple_map rows_multimap_;
};

} // namespace database
} // namespace libbitcoin

#endif
