/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/system.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/filter_database.hpp>
#include <bitcoin/database/databases/payment_database.hpp>
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
    typedef std::function<void(const system::code&)> result_handler;

    data_base(const settings& settings, bool catalog,
        bool neutrino_filter_support);

    // Open and close.
    // ------------------------------------------------------------------------

    /// Create and open all databases.
    bool create(const system::chain::block& genesis);

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

    /// Invalid if neutrino not initialized.
    const filter_database& neutrino_filters() const;

    /// Invalid if indexes not initialized.
    const payment_database& payments() const;

    // Node writers.
    // ------------------------------------------------------------------------

    // INITCHAIN (genesis)
    /// Push the block through candidacy and confirmation, without indexing.
    system::code push(const system::chain::block& block, size_t height=0,
        uint32_t median_time_past=0);

    // HEADER ORGANIZER (reorganize)
    /// Reorganize the header index to the specified fork point.
    system::code reorganize(const system::config::checkpoint& fork_point,
        system::header_const_ptr_list_const_ptr incoming,
        system::header_const_ptr_list_ptr outgoing);

    // BLOCK ORGANIZER (update)
    /// Update the stored block with txs.
    system::code update(const system::chain::block& block, size_t height);

    // BLOCK ORGANIZER (update, invalidate)
    /// Set header validation state and metadata.
    system::code invalidate(const system::chain::header& header,
        const system::code& error);

    // BLOCK ORGANIZER (candidate)
    /// Mark candidate block, txs and outputs spent by them as candidate.
    system::code candidate(const system::chain::block& block);

    // BLOCK ORGANIZER (candidate)
    /// Add transaction payments of the block to the payment index.
    system::code catalog(const system::chain::block& block);

    // BLOCK ORGANIZER (reorganize)
    /// Reorganize the block index to the specified fork point.
    system::code reorganize(const system::config::checkpoint& fork_point,
        system::block_const_ptr_list_const_ptr incoming,
        system::block_const_ptr_list_ptr outgoing);

    // BLOCK ORGANIZER (confirm)
    /// Confirm candidate block with confirmed parent.
    system::code confirm(const system::hash_digest& block_hash,
        size_t height);
    
    // TRANSACTION ORGANIZER (store)
    /// Store unconfirmed tx/payments that were verified with the given forks.
    system::code store(const system::chain::transaction& tx, uint32_t forks);

    // TRANSACTION ORGANIZER (store)
    /// Add transaction payment to the payment index.
    system::code catalog(const system::chain::transaction& tx);

protected:
    typedef std::function<system::code(const block_result&, file_offset&)>
        filter_key_extractor;

    void start();
    void commit();
    bool flush() const override;

    // Header reorganization.
    // ------------------------------------------------------------------------

    bool push_all(system::header_const_ptr_list_const_ptr headers,
        const system::config::checkpoint& fork_point);
    bool pop_above(system::header_const_ptr_list_ptr headers,
        const system::config::checkpoint& fork_point);
    system::code push_header(const system::chain::header& header, size_t height,
        uint32_t median_time_past);
    system::code pop_header(system::chain::header& out_header, size_t height);

    // Block reorganization.
    // ------------------------------------------------------------------------

    bool push_all(system::block_const_ptr_list_const_ptr blocks,
        const system::config::checkpoint& fork_point);
    bool pop_above(system::block_const_ptr_list_ptr blocks,
        const system::config::checkpoint& fork_point);
    system::code push_block(const system::chain::block& block, size_t height);
    system::code pop_block(system::chain::block& out_block, size_t height);

    // Neutrino filter update.
    // ------------------------------------------------------------------------

    system::code update_neutrino_filter(const system::chain::block& block);
    static system::code neutrino_filter_extractor(const block_result& result,
        file_offset& offset);

    // Filter checkpoint update.
    // ------------------------------------------------------------------------

    system::code initialize_filter_checkpoints(filter_database& database,
        filter_key_extractor extractor);

    system::code update_filter_checkpoints(filter_database& database,
        filter_key_extractor extractor,
        const system::config::checkpoint& fork_point,
        system::block_const_ptr_list_const_ptr incoming,
        system::block_const_ptr_list_ptr outgoing);

    // Databases.
    // ------------------------------------------------------------------------

    std::shared_ptr<block_database> blocks_;
    std::shared_ptr<transaction_database> transactions_;
    std::shared_ptr<filter_database> neutrino_filters_;
    std::shared_ptr<payment_database> payments_;

private:
    system::chain::transaction::list to_transactions(
        const block_result& result) const;

    std::atomic<bool> closed_;
    const bool catalog_;
    const bool neutrino_filter_support_;
    const settings& settings_;

    // Used to prevent unsafe concurrent writes.
    mutable system::shared_mutex write_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
