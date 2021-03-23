/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

// Sponsored in part by Digital Contract Design, LLC

#ifndef LIBBITCOIN_DATABASE_FITLER_DATABASE_HPP
#define LIBBITCOIN_DATABASE_FITLER_DATABASE_HPP

#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/result/filter_result.hpp>

namespace libbitcoin {
namespace database {

/// This enables lookups of filters and filter headers by block hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API filter_database
{
public:
    typedef boost::filesystem::path path;

    /// Construct the database.
    filter_database(const path& map_filename, size_t table_minimum,
        uint32_t buckets, size_t expansion, uint8_t filter_type);

    /// Close the database (all threads must first be stopped).
    ~filter_database();

    // Startup and shutdown.
    // ------------------------------------------------------------------------

    /// Initialize a new filter database.
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

    /// Fetch filter by its link.
    filter_result get(file_offset link) const;

    /// Fetch filter by its block hash.
    // Disabled due to optimization removing block_hash.
    // Currently stores header as hash, not used for retrieval.
    // filter_result get(const system::hash_digest& hash) const;

    system::hash_list checkpoints() const;

    bool set_checkpoints(system::hash_list&& checkpoints);

    // Writers.
    // ------------------------------------------------------------------------

    /// Store a filter and filter header associated with a block hash.
    bool store(const system::chain::block_filter& block_filter);

private:
    typedef system::hash_digest key_type;
    typedef array_index index_type;
    typedef file_offset link_type;
    typedef slab_manager<link_type> manager_type;
    typedef hash_table<manager_type, index_type, link_type, key_type> slab_map;

    // Store filter data.
    //-------------------------------------------------------------------------
    bool storize(const system::chain::block_filter& block_filter);

    // Identifier of the filters being stored (assumes uniform storage).
    uint8_t filter_type_;

    // TODO: eliminate hash table.
    // Hash table used for looking up filters by block hash.
    file_storage hash_table_file_;
    slab_map hash_table_;

    // This provides atomicity for height and position.
    mutable system::shared_mutex metadata_mutex_;

    system::atomic<system::hash_list> checkpoints_;
};

} // namespace database
} // namespace libbitcoin

#endif
