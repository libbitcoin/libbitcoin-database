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
#ifndef LIBBITCOIN_DATABASE_SPEND_DATABASE_HPP
#define LIBBITCOIN_DATABASE_SPEND_DATABASE_HPP

#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/slab_hash_table.hpp>

namespace libbitcoin {
namespace database {

/// This enables you to lookup the spend of an output point, returning
/// the input point. It is a simple map.
class BCD_API spend_database
{
public:
    typedef boost::filesystem::path path;

    /// Construct the database.
    spend_database(const path& filename, size_t buckets, size_t expansion);

    /// Close the database (all threads must first be stopped).
    ~spend_database();

    // Startup and shutdown.
    // ------------------------------------------------------------------------

    /// Initialize a new spend database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Commit latest inserts.
    void commit();

    /// Flush the memory map to disk.
    bool flush() const;

    /// Call to unload the memory map.
    bool close();

    // Queries.
    //-------------------------------------------------------------------------

    /// Get inpoint that spent the given outpoint.
    chain::input_point get(const chain::output_point& outpoint) const;

    /////// Return statistical info about the database.
    ////spend_statinfo statinfo() const;

    // Store.
    //-------------------------------------------------------------------------

    /// Store a spend in the database.
    void store(const chain::output_point& outpoint,
        const chain::input_point& spend);

    // Update.
    //-------------------------------------------------------------------------

    /// Delete outpoint spend item from database.
    bool unlink(const chain::output_point& outpoint);

private:
    ////typedef chain::point key_type;
    typedef short_hash key_type;
    typedef array_index index_type;
    typedef array_index link_type;
    typedef record_manager<link_type> record_manager;
    typedef slab_hash_table<record_manager, key_type, index_type, link_type> record_map;

    // Hash table used for looking up inpoint spends by outpoint.
    file_storage hash_table_file_;
    record_map hash_table_;
};

} // namespace database
} // namespace libbitcoin

#endif
