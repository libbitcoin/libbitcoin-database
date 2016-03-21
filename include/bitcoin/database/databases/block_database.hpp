/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_BLOCK_DATABASE_HPP
#define LIBBITCOIN_DATABASE_BLOCK_DATABASE_HPP

#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/slab_hash_table.hpp>
#include <bitcoin/database/result/block_result.hpp>

namespace libbitcoin {
namespace database {

/// Stores block_headers each with a list of transaction indexes.
/// Lookup possible by hash or height.
class BCD_API block_database
{
public:
    block_database(const boost::filesystem::path& map_filename,
        const boost::filesystem::path& index_filename);

    /// Initialize a new transaction database.
    void create();

    /// Call before using the database.
    void start();

    /// Call stop to unload the memory map.
    bool stop();

    /// Fetch block by height using the index table.
    block_result get(size_t height) const;

    /// Fetch block by hash using the hashtable.
    block_result get(const hash_digest& hash) const;

    /// Store a block in the database.
    void store(const chain::block& block);

    /// Store a block in the database.
    void store(const chain::block& block, size_t height);

    /// Unlink all blocks upwards from (and including) from_height.
    void unlink(const size_t from_height);

    /// Synchronise storage with disk so things are consistent.
    /// Should be done at the end of every block write.
    void sync();

    /// Latest block height in our chain. Returns false if no blocks exist.
    /// This is actually the count of blocks minus one and does not represent
    /// the logical top if there are gaps in the chain. Use gap to validate
    /// the top at startup
    bool top(size_t& out_height) const;

private:
    typedef slab_hash_table<hash_digest> slab_map;

    /// Write position of tx.
    void write_position(const file_offset position);

    /// Use intermediate records table to find blk position from height.
    file_offset read_position(const array_index index) const;

    /// Hash table used for looking up blocks by hash.
    memory_map lookup_file_;
    slab_hash_table_header lookup_header_;
    slab_manager lookup_manager_;
    slab_map lookup_map_;

    /// Table used for looking up blocks by height.
    /// Resolves to a position within the slab.
    memory_map index_file_;
    record_manager index_manager_;
};

} // namespace database
} // namespace libbitcoin

#endif
