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
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Construct the database.
    transaction_database(const path& map_filename, size_t buckets,
        size_t expansion, mutex_ptr mutex = nullptr);

    /// Close the database (all threads must first be stopped).
    ~transaction_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Fetch transaction by its hash.
    transaction_result get(const hash_digest& hash) const;

    /// Fetch transaction by its hash, at or below the specified block height.
    transaction_result get(const hash_digest& hash, size_t fork_height) const;

    /// Store a transaction in the database.
    void store(size_t height, size_t position, const chain::transaction& tx);

    /// Update the spender height of the output in the tx store.
    bool update(const chain::output_point& point, size_t spender_height);

    /// Delete a transaction from database.
    bool unlink(const hash_digest& hash);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush();

private:
    typedef slab_hash_table<hash_digest> slab_map;

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    // Hash table used for looking up txs by hash.
    memory_map lookup_file_;
    slab_hash_table_header lookup_header_;
    slab_manager lookup_manager_;
    slab_map lookup_map_;
};

} // namespace database
} // namespace libbitcoin

#endif
