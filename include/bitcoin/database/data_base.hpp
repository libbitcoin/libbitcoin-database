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
#ifndef LIBBITCOIN_DATABASE_DATA_BASE_HPP
#define LIBBITCOIN_DATABASE_DATA_BASE_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/databases/history_database.hpp>
#include <bitcoin/database/databases/stealth_database.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe and implements the sequential locking pattern.
class BCD_API data_base
  : public store
{
public:
    typedef handle0 result_handler;
    typedef boost::filesystem::path path;

    data_base(const settings& settings);

    // Open and close.
    // ------------------------------------------------------------------------

    /// Create and open all databases.
    bool create(const chain::block& genesis);

    /// Open all databases.
    bool open() override;

    /// Close all databases.
    bool close() override;

    /// Call close on destruct.
    ~data_base();

    /// Reader interfaces.
    // ------------------------------------------------------------------------
    // These are const to preclude write operations.

    const block_database& blocks() const;

    const transaction_database& transactions() const;

    /// Invalid if indexes not initialized.
    const history_database& history() const;

    /// Invalid if indexes not initialized.
    const stealth_database& stealth() const;

    // Utility writers.
    // ------------------------------------------------------------------------
    // Block push/pop are not used by blockchain.

    /// Push next top block of expected height.
    code push(const chain::block& block, size_t height);

    /// Push next top header of expected height.
    code push(const chain::header& header, size_t height);

    /// Pop top block of expected height.
    code pop(chain::block& out_block, size_t height);

    /// Pop top header of expected height.
    code pop(chain::header& out_header, size_t height);

    // Organization.
    // ------------------------------------------------------------------------

    /// Push unconfirmed tx that was verified with the given forks.
    code push(const chain::transaction& tx, uint32_t forks);

    /// Update the block with transactions.
    code update(block_const_ptr block, size_t height);

    /// Reorganize the header index, updating block index accordingly.
    void reorganize(const config::checkpoint& fork_point,
        header_const_ptr_list_const_ptr incoming,
        header_const_ptr_list_ptr outgoing, dispatcher& dispatch,
        result_handler handler);

protected:
    void start();
    void commit();
    bool flush() const override;

    // Utilities.
    // ------------------------------------------------------------------------

    code verify(const config::checkpoint& fork_point, bool block_index) const;
    code verify_top(size_t height, bool block_index) const;
    code verify_push(const chain::transaction& tx) const;
    code verify_push(const chain::header& header, size_t height) const;
    code verify_push(const chain::block& block, size_t height) const;
    code verify_update(const chain::block& block, size_t height) const;
    chain::transaction::list to_transactions(const block_result& result) const;

    // Synchronous.
    // ------------------------------------------------------------------------

    void push_inputs(const chain::transaction& tx, size_t height);
    void push_outputs(const chain::transaction& tx, size_t height);
    void push_stealth(const chain::transaction& tx, size_t height);
    code push_transactions(const chain::block& block, size_t height,
        uint32_t median_time_past, size_t bucket=0, size_t buckets=1,
        transaction_state state=transaction_state::confirmed);

    bool pop_inputs(const chain::transaction& tx);
    bool pop_outputs(const chain::transaction& tx);
    bool pop_stealth(const chain::transaction& tx);
    code pop_transactions(const chain::block& out_block, size_t bucket=0,
        size_t buckets=1);

    // Header Reorganization (not parallel).
    // ------------------------------------------------------------------------

    bool pop_above(header_const_ptr_list_ptr headers,
        const config::checkpoint& fork_point, dispatcher& dispatch,
        result_handler handler);
    bool push_all(header_const_ptr_list_const_ptr headers,
        const config::checkpoint& fork_point, dispatcher& dispatch,
        result_handler handler);

    // Databases.
    // ------------------------------------------------------------------------

    std::shared_ptr<block_database> blocks_;
    std::shared_ptr<transaction_database> transactions_;
    std::shared_ptr<history_database> history_;
    std::shared_ptr<stealth_database> stealth_;

private:
    typedef chain::input::list inputs;
    typedef chain::output::list outputs;

    // Update, validate and confirm the genesis block.
    code update_genesis(const chain::block& block);

    std::atomic<bool> closed_;
    const settings& settings_;

    // Used to prevent concurrent unsafe writes.
    mutable shared_mutex write_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
