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
#include <bitcoin/database/block_state.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/result/block_result.hpp>

namespace libbitcoin {
namespace database {

/// Stores block_headers each with a list of transaction indexes.
/// Lookup possible by hash or height.
class BCD_API block_database
{
public:
    typedef boost::filesystem::path path;

    /// Construct the database.
    block_database(const path& map_filename,
        const path& candidate_index_filename,
        const path& confirmed_index_filename, const path& tx_index_filename,
        size_t buckets, size_t expansion, const bc::settings& bitcoin_settings);

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

    /// The height of the highest candidate|confirmed block.
    bool top(size_t& out_height, bool candidate) const;

    /// Fetch block by block|header index height.
    block_result get(size_t height, bool candidate) const;

    /// Fetch block by hash.
    block_result get(const hash_digest& hash) const;

    /// Populate header metadata for the given header.
    void get_header_metadata(const chain::header& header) const;

    // Writers.
    // ------------------------------------------------------------------------

    /// Push header, validated at height.
    void push(const chain::header& header, size_t height);

    /// Populate pooled block transaction references, state is unchanged.
    bool update(const chain::block& block);

    /// Promote pooled block to valid|invalid and set code.
    bool validate(const hash_digest& hash, const code& error);

    /// Promote pooled|candidate block to candidate|confirmed respectively.
    bool confirm(const hash_digest& hash, size_t height, bool candidate);

    /// Demote candidate|confirmed header to pooled|pooled (not candidate).
    bool unconfirm(const hash_digest& hash, size_t height, bool candidate);

private:
    typedef hash_digest key_type;
    typedef array_index link_type;
    typedef record_manager<link_type> manager_type;
    typedef list_element<const manager_type, link_type, key_type> const_element;
    typedef hash_table<manager_type, array_index, link_type, key_type> record_map;

    typedef message::compact_block::short_id_list short_id_list;

    uint8_t confirm(const_element& element, bool positive, bool candidate);
    link_type associate(const chain::transaction::list& transactions);
    void store(const chain::header& header, size_t height,
        uint32_t median_time_past, uint32_t checksum, link_type tx_start,
        size_t tx_count, uint8_t status);

    // Index Utilities.
    bool read_top(size_t& out_height, const manager_type& manager) const;
    link_type read_index(size_t height, const manager_type& manager) const;
    void pop_index(size_t height, manager_type& manager);
    void push_index(link_type index, size_t height, manager_type& manager);

    static const size_t prefix_size_;

    // Hash table used for looking up block headers by hash.
    file_storage hash_table_file_;
    record_map hash_table_;

    // Table used for looking up candidate headers by height.
    file_storage candidate_index_file_;
    manager_type candidate_index_;

    // Table used for looking up confirmed headers by height.
    file_storage confirmed_index_file_;
    manager_type confirmed_index_;

    // Association table between blocks and their contained transactions.
    // Only first tx is indexed and count is required to read the full set.
    // This indexes txs (vs. blocks) so the link type may be differentiated.
    file_storage tx_index_file_;
    manager_type tx_index_;

    const bc::settings& bitcoin_settings_;

    // This provides atomicity for checksum, tx_start, tx_count, state.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
