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
#include <functional>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/databases/address_database.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>

namespace libbitcoin {
namespace database {

/// This class provides thread safe access to the database.
class BCD_API data_base
  : public store
{
public:
    typedef std::function<void(const code&)> result_handler;

    data_base(const settings& settings);

    // Open and close.
    // ------------------------------------------------------------------------

    /// Create and open all databases.
    bool create(const chain::block& genesis);

    // Expose polymorphic create method from base.
    using store::create;

    /// Open all databases.
    bool open() override;

    /// Close all databases.
    bool close() override;

    /// Call close on destruct.
    ~data_base();

    /// Reader interfaces.
    // ------------------------------------------------------------------------
    // These are const to preclude write operations by public callers.

    const block_database& blocks() const;

    const transaction_database& transactions() const;

    /// Invalid if indexes not initialized.
    const address_database& addresses() const;

    // Node writers.
    // ------------------------------------------------------------------------

    // INITCHAIN (genesis)
    /// Push the block through candidacy and confirmation, without indexing.
    code push(const chain::block& block, size_t height=0,
        uint32_t median_time_past=0);

    // HEADER ORGANIZER (reorganize)
    /// Reorganize the header index to the specified fork point.
    code reorganize(const config::checkpoint& fork_point,
        header_const_ptr_list_const_ptr incoming,
        header_const_ptr_list_ptr outgoing);

    // BLOCK ORGANIZER (update)
    /// Update the stored block with txs.
    code update(const chain::block& block, size_t height);

    // BLOCK ORGANIZER (update, invalidate)
    /// Set header validation state and metadata.
    code invalidate(const chain::header& header, const code& error);

    // BLOCK ORGANIZER (candidate)
    /// Mark candidate block, txs and outputs spent by them as candidate.
    code candidate(const chain::block& block);

    // BLOCK ORGANIZER (candidate)
    /// Add payments of transactions of the block to the payment index.
    code index(const chain::block& block);

    // BLOCK ORGANIZER (reorganize)
    /// Reorganize the block index to the specified fork point.
    code reorganize(const config::checkpoint& fork_point,
        block_const_ptr_list_const_ptr incoming,
        block_const_ptr_list_ptr outgoing);

    // TRANSACTION ORGANIZER (store)
    /// Store unconfirmed tx/payments that was verified with the given forks.
    code store(const chain::transaction& tx, uint32_t forks);

    // TRANSACTION ORGANIZER (store)
    /// Add payments of the transaction to the payment index.
    code index(const chain::transaction& tx);

protected:
    void start();
    void commit();
    bool flush() const override;

    // Header reorganization.
    // ------------------------------------------------------------------------

    bool push_all(header_const_ptr_list_const_ptr headers,
        const config::checkpoint& fork_point);
    bool pop_above(header_const_ptr_list_ptr headers,
        const config::checkpoint& fork_point);
    code push_header(const chain::header& header, size_t height,
        uint32_t median_time_past);
    code pop_header(chain::header& out_header, size_t height);

    // Block reorganization.
    // ------------------------------------------------------------------------

    bool push_all(block_const_ptr_list_const_ptr blocks,
        const config::checkpoint& fork_point);
    bool pop_above(block_const_ptr_list_ptr headers,
        const config::checkpoint& fork_point);
    code push_block(const chain::block& block, size_t height);
    code pop_block(chain::block& out_block, size_t height);


    // Databases.
    // ------------------------------------------------------------------------

    std::shared_ptr<block_database> blocks_;
    std::shared_ptr<transaction_database> transactions_;
    std::shared_ptr<address_database> addresses_;

private:
    chain::transaction::list to_transactions(const block_result& result) const;

    std::atomic<bool> closed_;
    const settings& settings_;

    // Used to prevent unsafe concurrent writes.
    mutable shared_mutex write_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
