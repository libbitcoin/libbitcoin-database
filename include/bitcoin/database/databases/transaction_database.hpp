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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_DATABASE_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_DATABASE_HPP

#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/state/transaction_state.hpp>
#include <bitcoin/database/unspent_outputs.hpp>

namespace libbitcoin {
namespace database {

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API transaction_database
{
public:
    typedef boost::filesystem::path path;

    /// Construct the database.
    transaction_database(const path& map_filename, size_t buckets,
        size_t expansion, size_t cache_capacity);

    /// Close the database (all threads must first be stopped).
    ~transaction_database();

    // Startup and shutdown.
    // ------------------------------------------------------------------------

    /// Initialize a new transaction database.
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

    /// Fetch transaction by its link.
    transaction_result get(file_offset link) const;

    /// Fetch transaction by its hash.
    transaction_result get(const hash_digest& hash) const;

    /// Populate output metadata for the specified point.
    /// Confirmation is satisfied by confirmed|indexed, fork point dependent.
    bool get_output(const chain::output_point& point,
        size_t fork_height=max_size_t) const;

    // Store.
    // ------------------------------------------------------------------------

    /// Height and position may be sentinels or otherwise.
    /// Store|promote the transaction and set link metadata.
    bool store(const chain::transaction& tx, size_t height,
        uint32_t median_time_past, size_t position,
        transaction_state state=transaction_state::pooled);

    // Demote the transaction to pooled.
    bool unconfirm(const chain::transaction& tx);

private:
    typedef hash_digest key_type;
    typedef array_index index_type;
    typedef file_offset link_type;
    typedef slab_manager<link_type> slab_manager;
    typedef hash_table<slab_manager, key_type, index_type, link_type> slab_map;

    // Update the spender height of the output.
    bool spend(const chain::output_point& point, size_t spender_height);

    // Update the state of the existing tx.
    bool confirm(link_type link, size_t height, uint32_t median_time_past,
        size_t position, transaction_state state);

    // Demote the transaction to pooled.
    bool unconfirm(link_type link);

    // Hash table used for looking up txs by hash.
    file_storage hash_table_file_;
    slab_map hash_table_;

    // This is thread safe, and as a cache is mutable.
    mutable unspent_outputs cache_;

    // This provides atomicity for height and position.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
