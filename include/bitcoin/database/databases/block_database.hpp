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

    /// Construct the database.
    block_database(const path& map_filename, const path& header_index_filename,
        const path& block_index_filename, const path& tx_index_filename,
        size_t buckets, size_t expansion, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~block_database();

    // Startup and shutdown.
    // ------------------------------------------------------------------------

    /// Initialize a new transaction database.
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

    /// The height of the highest indexed block|header.
    bool top(size_t& out_height, bool block_index=true) const;

    /// Fetch block by block|header index height.
    block_result get(size_t height, bool block_index=true) const;

    /// Fetch block by hash.
    block_result get(const hash_digest& hash) const;

    // Store.
    // ------------------------------------------------------------------------

    // Store header, indexed at the specified height.
    void store(const chain::header& header, size_t height);

    /// Store block, indexed at the specified height, and associate tx offsets.
    void store(const chain::block& block, size_t height);

    // Update.
    // ------------------------------------------------------------------------

    /// Promote block to indexed|confirmed.
    bool confirm(const hash_digest& hash, size_t height, bool block_index);

    /// Demote header|block at the given height to pooled.
    bool unconfirm(const hash_digest& hash, size_t height, bool block_index);

private:
    typedef record_hash_table<hash_digest> record_map;
    typedef message::compact_block::short_id_list short_id_list;

    array_index associate(const chain::transaction::list& transactions);
    void store(const chain::header& header, size_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, uint8_t status);

    // Index Utilities.
    bool read_top(size_t& out_height, const record_manager& manager) const;
    array_index read_index(size_t height, const record_manager& manager) const;
    void pop_index(size_t height, record_manager& manager);
    void push_index(array_index index, size_t height,
        record_manager& manager);

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    // Hash table used for looking up block headers by hash.
    memory_map lookup_file_;
    record_hash_table_header lookup_header_;
    record_manager lookup_manager_;
    record_map lookup_map_;

    // Table used for looking up headers by height.
    // Each record resolves to a record via array_index.
    memory_map header_index_file_;
    record_manager header_index_manager_;

    // Table used for looking up blocks by height.
    // Each record resolves to a record via array_index.
    memory_map block_index_file_;
    record_manager block_index_manager_;

    // Association table between blocks and their contained transactions.
    // Each record resolves to a record via array_index.
    memory_map tx_index_file_;
    record_manager tx_index_manager_;

    // This provides atomicity for checksum, tx_start, tx_count, state.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
