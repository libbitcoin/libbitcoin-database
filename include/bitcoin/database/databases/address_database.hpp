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
#ifndef LIBBITCOIN_DATABASE_ADDRESS_DATABASE_HPP
#define LIBBITCOIN_DATABASE_ADDRESS_DATABASE_HPP

#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/hash_table_multimap.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

/// This is a record multimap where the key is the Bitcoin address hash,
/// which returns several rows giving the history for that address.
class BCD_API address_database
{
public:
    typedef boost::filesystem::path path;
    typedef chain::payment_record::list list;

    /// Construct the database.
    address_database(const path& lookup_filename, const path& rows_filename,
        size_t buckets, size_t expansion);

    /// Close the database (all threads must first be stopped).
    ~address_database();

    // Startup and shutdown.
    // ----------------------------------------------------------------------------

    /// Initialize a new address database.
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

    /////// Return statistical info about the database.
    ////address_statinfo statinfo() const;

    // Store.
    //-------------------------------------------------------------------------

    /// Add a row for the key. If key doesn't exist it will be created.
    void store(const short_hash& key, const chain::payment_record& payment);

    // Update.
    //-------------------------------------------------------------------------

    /// Logically delete the last row that was added to key.
    bool unlink_last_row(const short_hash& key);

private:
    typedef short_hash key_type;
    typedef array_index index_type;
    typedef array_index link_type;
    typedef record_manager<link_type> record_manager;
    typedef hash_table<record_manager, key_type, index_type, link_type>
        record_map;

    // The record multimap as distinct file as opposed to linkage within the map
    // allows avoidance of hash storage with each entry. This is similar to
    // the transaction index with the exception that the tx index stores tx
    // sets by block in a contiguous array, eliminating a need for linked list.
    typedef hash_table_multimap<key_type, index_type, link_type> record_multimap;

    /// Hash table used for start index lookup for linked list by address hash.
    file_storage hash_table_file_;
    record_map hash_table_;

    /// History rows.
    file_storage address_file_;
    record_manager address_index_;
    record_multimap address_multimap_;
};

} // namespace database
} // namespace libbitcoin

#endif
