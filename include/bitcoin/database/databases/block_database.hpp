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
#ifndef LIBBITCOIN_DATABASE_BLOCK_DATABASE_HPP
#define LIBBITCOIN_DATABASE_BLOCK_DATABASE_HPP

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/primitives/record_hash_table.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/result/block_result.hpp>

namespace libbitcoin {
namespace database {

/// Stores block_headers each with a list of transaction indexes.
/// Lookup possible by hash or height.
class BCD_API block_database
{
public:
    typedef std::vector<size_t> heights;
    typedef boost::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    static const array_index empty;

    /// Construct the database.
    block_database(const path& map_filename, const path& block_index_filename,
        const path& tx_index_filename, size_t buckets, size_t expansion,
        mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~block_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory maps to disk.
    bool flush() const;

    // Queries.
    //-------------------------------------------------------------------------

    /// Determine if a block exists at the given height.
    bool exists(size_t height) const;

    /// The list of heights representing all chain gaps.
    bool gaps(heights& out_gaps) const;

    /// Fetch block by height using the index table.
    block_result get(size_t height) const;

    /// Fetch block by hash using the hashtable.
    block_result get(const hash_digest& hash, bool require_confirmed) const;

    /// This is ordered, but block parallelism may leave confirmation gaps.
    /// Store an unconfirmed header with no transactions.
    void store(const chain::header& header, size_t height);

    /// This is optimized by storing tx file offsets in metadata.
    /// Store a header and associate transactions (false if any missing).
    void store(const chain::block& block, size_t height, bool confirmed);

    /// TODO: optimize by storing tx file offsets in metadata.
    /// This may come from the wire or be generated via the mining interface.
    /// Store a header and associate transactions (false if any missing).
    void store(const message::compact_block& compact, size_t height,
        bool confirmed);

    /// Update an existing block's transactions association.
    bool update(const chain::block& block, size_t height, bool confirmed);

    /// Update an existing block's transactions association.
    bool update(const message::compact_block& compact, size_t height,
        bool confirmed);

    /// Promote the block and all ancestors up to the fork point.
    /// This does not promote the block's transactions or their spends, which
    /// must be confirmed before this call.
    bool confirm(const hash_digest& hash, bool confirm=true);

    /// Demote all blocks at and above the given height.
    /// This does not demote the blocks' transactions or their spends, which
    /// must be unconfirmed before this call. Should always be the top block.
    bool unconfirm(size_t from_height);

    /// The index of the highest existing block, independent of gaps.
    bool top(size_t& out_height) const;

private:
    typedef record_hash_table<hash_digest> record_map;
    typedef message::compact_block::short_id_list short_id_list;

    // Associate an array of transactions for a block.
    array_index associate(const chain::transaction::list& transactions);
    array_index associate(const short_id_list& ids);

    void store(const chain::header& header, size_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, bool confirmed);

    bool update(const hash_digest& hash, size_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, bool confirmed);

    // Zeroize the specified index values.
    void zeroize(array_index first, array_index count);

    // Write block hash table index into the block index.
    void write_index(array_index index, array_index height);

    // Use block index to get block hash table index from height.
    array_index get_index(array_index height) const;

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    // Hash table used for looking up block headers by hash.
    memory_map lookup_file_;
    record_hash_table_header lookup_header_;
    record_manager lookup_manager_;
    record_map lookup_map_;

    // Table used for looking up blocks by height.
    // Each record resolves to a record via array_index.
    memory_map block_index_file_;
    record_manager block_index_manager_;

    // Association table between blocks and their contained transactions.
    // Each record resolves to a record via array_index.
    memory_map tx_index_file_;
    record_manager tx_index_manager_;

    // Guard against concurrent update of a range of block indexes.
    mutable upgrade_mutex index_mutex_;

    // This provides atomicity for checksum, tx_start, tx_count, confirmed.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
