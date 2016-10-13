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
#ifndef LIBBITCOIN_DATABASE_DATA_BASE_HPP
#define LIBBITCOIN_DATABASE_DATA_BASE_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/spend_database.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/databases/history_database.hpp>
#include <bitcoin/database/databases/stealth_database.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/unicode/file_lock.hpp>

namespace libbitcoin {
namespace database {

typedef uint64_t handle;

/// This class is thread safe and implements the sequential locking pattern.
class BCD_API data_base
{
public:
    typedef boost::filesystem::path path;

    class store
    {
    public:
        store(const path& prefix);
        bool touch_all() const;

        path database_lock;
        path blocks_lookup;
        path blocks_index;
        path history_lookup;
        path history_rows;
        path stealth_rows;
        path spends_lookup;
        path transactions_lookup;
    };

    // Determine if the given handle is a write-locked handle.
    static bool is_write_locked(handle handle);

    /// Create a new database file with a given path prefix and default paths.
    static bool initialize(const path& prefix, const chain::block& genesis);
    static bool touch_file(const path& file_path);

    /// Construct all databases.
    data_base(const settings& settings);

    /// Stop all databases (threads must be joined).
    ~data_base();

    // Open and close.
    // ------------------------------------------------------------------------

    /// Create and open all databases.
    bool create();

    /// Open all databases.
    bool open();

    /// Close all databases.
    bool close();

    // Sequential locking.
    // ------------------------------------------------------------------------

    handle begin_read() const;
    bool is_read_valid(handle handle) const;

    bool begin_write();
    bool end_write();

    // Add and remove blocks.
    // ------------------------------------------------------------------------

    /// Store a block in the database.
    /// Returns false if a block already exists at height.
    bool insert(const chain::block& block, size_t height);

    /// Returns false if height is not the current top + 1 or not linked.
    bool push(const chain::block& block, size_t height);

    /// Returns false if first_height is not the current top + 1 or not linked.
    bool push(const chain::block::list& blocks, size_t first_height);

    /// Pop the set of blocks above the given hash.
    /// Returns true with empty list if height is empty.
    /// Returns false if the database is corrupt or the hash doesn't exit.
    /// Any blocks returned were successfully popped prior to any failure.
    bool pop_above(chain::block::list& out_blocks,
        const hash_digest& fork_hash);

protected:
    data_base(const store& paths, size_t history_height, size_t stealth_height);
    data_base(const path& prefix, size_t history_height, size_t stealth_height);

private:
    typedef chain::input::list inputs;
    typedef chain::output::list outputs;
    typedef std::atomic<size_t> sequential_lock;
    typedef bc::database::file_lock file_lock;

    static void uninitialize_lock(const path& lock);
    static file_lock initialize_lock(const path& lock);

    void push_transactions(const chain::block& block, size_t height);
    void push_inputs(const hash_digest& tx_hash, size_t height,
        const inputs& inputs);
    void push_outputs(const hash_digest& tx_hash, size_t height,
        const outputs& outputs);
    void push_stealth(const hash_digest& tx_hash, size_t height,
        const outputs& outputs);

    chain::block pop();
    void pop_inputs(const inputs& inputs, size_t height);
    void pop_outputs(const outputs& outputs, size_t height);

    void synchronize();

    const path lock_file_path_;
    const size_t history_height_;
    const size_t stealth_height_;

    // Atomic counter for implementing the sequential lock pattern.
    sequential_lock sequential_lock_;

    // Allows us to restrict database access to our process (or fail).
    std::shared_ptr<file_lock> file_lock_;

    // Cross-database mutext to prevent concurrent file remapping.
    std::shared_ptr<shared_mutex> mutex_;

public:

    /// Individual database query engines.
    block_database blocks;
    history_database history;
    spend_database spends;
    stealth_database stealth;
    transaction_database transactions;
};

} // namespace database
} // namespace libbitcoin

#endif
