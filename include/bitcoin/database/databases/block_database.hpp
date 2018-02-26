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

#include <atomic>
#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/result/block_result.hpp>
#include <bitcoin/database/state/block_state.hpp>

namespace libbitcoin {
namespace database {

/// Stores block_headers each with a list of transaction indexes.
/// Lookup possible by hash or height.
class BCD_API block_database
{
public:
    typedef boost::filesystem::path path;

    /// Construct the database.
    block_database(const path& map_filename, const path& header_index_filename,
        const path& block_index_filename, const path& tx_index_filename,
        size_t buckets, size_t expansion);

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

    /// The highest confirmed block of the header index.
    size_t fork_point() const;

    /// The highest valid block of the header index.
    size_t valid_point() const;

    /// The height of the highest indexed block|header.
    bool top(size_t& out_height, bool block_index=true) const;

    /// Fetch block by block|header index height.
    block_result get(size_t height, bool block_index=true) const;

    /// Fetch block by hash.
    block_result get(const hash_digest& hash) const;

    // Store.
    // ------------------------------------------------------------------------

    /// Push header, validated at height.
    void push(const chain::header& header, size_t height);

    /// Push block, validated at height, and associate tx links.
    void push(const chain::block& block, size_t height,
        uint32_t median_time_past);

    // Update.
    // ------------------------------------------------------------------------

    /// Populate pent block transaction references, state is unchanged.
    bool update(const chain::block& block);

    /// Promote pent block to valid|invalid.
    bool validate(const hash_digest& hash, bool positive);

    /// Promote pooled|indexed block to indexed|confirmed.
    bool confirm(const hash_digest& hash, size_t height, bool block_index);

    /// Demote header|block at the given height to pooled.
    bool unconfirm(const hash_digest& hash, size_t height, bool block_index);

private:
    typedef hash_digest key_type;
    typedef array_index link_type;
    typedef record_manager<link_type> manager_type;
    typedef list_element<const manager_type, link_type, key_type> const_element;
    typedef hash_table<manager_type, array_index, link_type, key_type> record_map;

    typedef message::compact_block::short_id_list short_id_list;

    uint8_t confirm(const_element& element, bool positive, bool block_index);
    link_type associate(const chain::transaction::list& transactions);
    void push(const chain::header& header, size_t height,
        uint32_t median_time_past, uint32_t checksum, link_type tx_start,
        size_t tx_count, uint8_t status);

    // Index Utilities.
    bool read_top(size_t& out_height, const manager_type& manager) const;
    link_type read_index(size_t height, const manager_type& manager) const;
    void pop_index(size_t height, manager_type& manager);
    void push_index(link_type index, size_t height, manager_type& manager);

    static const size_t prefix_size_;

    // The top confirmed block in the header index.
    std::atomic<size_t> fork_point_;

    // The top valid block in the header index.
    std::atomic<size_t> valid_point_;

    // Hash table used for looking up block headers by hash.
    file_storage hash_table_file_;
    record_map hash_table_;

    // Table used for looking up headers by height.
    file_storage header_index_file_;
    manager_type header_index_;

    // Table used for looking up blocks by height.
    file_storage block_index_file_;
    manager_type block_index_;

    // Association table between blocks and their contained transactions.
    // Only first tx is indexed and count is required to read the full set.
    // This indexes txs (vs. blocks) so the link type may be differentiated.
    file_storage tx_index_file_;
    manager_type tx_index_;

    // This provides atomicity for checksum, tx_start, tx_count, state.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
