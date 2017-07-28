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
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/primitives/slab_hash_table.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/unspent_outputs.hpp>

namespace libbitcoin {
namespace database {

// Stored txs are verified or protected by valid header PoW, states are:
// TODO: compress into position using flag for indexed and sentinal for pool.
enum class transaction_state : uint8_t
{
    /// Interface only (not stored).
    not_found = 0,

    /// If the tx can become valid via soft fork, set "stored" instead.
    /// Retain for reject, height/position unused (is this usable?).
    invalid = 1,

    /// Valid via header PoW only, height/position unused.
    stored = 2,

    /// Valid via pool|popped block, height is forks, position unused.
    pooled = 3,

    /// Valid, header-indexed, height is forks, position unused.
    indexed = 4,

    /// Valid, block-indexed, height is height, position position.
    confirmed = 5
};

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API transaction_database
{
public:
    typedef boost::filesystem::path path;
    typedef slab_hash_table<hash_digest> slab_map;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Sentinel for use in tx position to indicate unconfirmed.
    static const size_t unconfirmed;

    /// Construct the database.
    transaction_database(const path& map_filename, size_t buckets,
        size_t expansion, size_t cache_capacity, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~transaction_database();

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

    /// Fetch transaction by file offset.
    transaction_result get(file_offset offset) const;

    /// Fetch transaction by its hash, at or below the specified block height.
    transaction_result get(const hash_digest& hash,
        size_t fork_height=max_size_t, bool require_confirmed=false) const;

    /// Get the output at the specified index within the transaction.
    bool get_output(chain::output& out_output, size_t& out_height,
        uint32_t& out_median_time_past, bool& out_coinbase,
        const chain::output_point& point, size_t fork_height,
        bool require_confirmed) const;

    // Store.
    // ------------------------------------------------------------------------

    /// Height and position may be sentinels or otherwise.
    /// Store a transaction in the database, returning slab file offset.
    file_offset store(const chain::transaction& tx, size_t height,
        uint32_t median_time_past, size_t position);

    // Update.
    //-------------------------------------------------------------------------

    /// Update the spender height of the output in the tx store.
    bool spend(const chain::output_point& point, size_t spender_height);

    /// Update the spender height of the output in the tx store.
    bool unspend(const chain::output_point& point);

    /// Promote an unconfirmed tx (not including its indexes).
    file_offset confirm(const hash_digest& hash, size_t height,
        uint32_t median_time_past, size_t position);

    /// Demote the transaction.
    bool unconfirm(const hash_digest& hash);

private:
    static bool is_confirmed(transaction_state status);
    static bool is_indexed(transaction_state status);
    static bool is_pooled(transaction_state status);
    static bool is_valid(transaction_state position);
    static transaction_state to_status(bool confirmed);

    memory_ptr find(const hash_digest& hash, size_t maximum_height,
        bool require_confirmed) const;

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    // Hash table used for looking up txs by hash.
    memory_map lookup_file_;
    slab_hash_table_header lookup_header_;
    slab_manager lookup_manager_;
    slab_map lookup_map_;

    // This is thread safe, and as a cache is mutable.
    mutable unspent_outputs cache_;

    // This provides atomicity for height and position.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
