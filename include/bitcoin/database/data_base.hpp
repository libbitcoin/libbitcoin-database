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
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/spend_database.hpp>
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
    typedef store::handle handle;
    typedef handle0 result_handler;
    typedef boost::filesystem::path path;
    typedef message::block::const_ptr block_const_ptr;
    typedef message::block::const_ptr_list block_const_ptr_list;
    typedef message::block::const_ptr_list_ptr block_const_ptr_list_ptr;
    typedef message::block::const_ptr_list_const_ptr 
        block_const_ptr_list_const_ptr;

    // Construct.
    // ----------------------------------------------------------------------------

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

    // Readers.
    // ------------------------------------------------------------------------

    const block_database& blocks() const;

    const transaction_database& transactions() const;

    /// Invalid if indexes not initialized.
    const spend_database& spends() const;

    /// Invalid if indexes not initialized.
    const history_database& history() const;

    /// Invalid if indexes not initialized.
    const stealth_database& stealth() const;

    // Synchronous writers.
    // ------------------------------------------------------------------------

    /// Store a block in the database.
    /// Returns false if a block already exists at height.
    code insert(const chain::block& block, size_t height);

    /// Returns false if height is not the current top + 1 or not linked.
    code push(const chain::block& block, size_t height);

    // Asynchronous writers.
    // ------------------------------------------------------------------------

    /// Sets error if first_height is not the current top + 1 or not linked.
    void push_all(block_const_ptr_list_const_ptr in_blocks,
        size_t first_height, dispatcher& dispatch, result_handler handler);

    /// Pop the set of blocks above the given hash.
    /// Sets error if the database is corrupt or the hash doesn't exit.
    /// Any blocks returned were successfully popped prior to any failure.
    void pop_above(block_const_ptr_list_ptr out_blocks,
        const hash_digest& fork_hash, dispatcher& dispatch,
        result_handler handler);

protected:
    void start();
    void synchronize();
    bool flush() override;

    std::shared_ptr<block_database> blocks_;
    std::shared_ptr<transaction_database> transactions_;
    std::shared_ptr<spend_database> spends_;
    std::shared_ptr<history_database> history_;
    std::shared_ptr<stealth_database> stealth_;

private:
    typedef chain::input::list inputs;
    typedef chain::output::list outputs;

    // Synchronous writers.
    // ------------------------------------------------------------------------
    void push_transactions(const chain::block& block, size_t height,
        size_t bucket=0, size_t buckets=1);
    void push_updates(const chain::transaction& tx, size_t height,
        bool coinbase);
    void push_heights(size_t height, const chain::input::list& inputs);
    void push_inputs(const hash_digest& tx_hash, size_t height,
        const inputs& inputs);
    void push_outputs(const hash_digest& tx_hash, size_t height,
        const outputs& outputs);
    void push_stealth(const hash_digest& tx_hash, size_t height,
        const outputs& outputs);

    chain::block pop();
    void pop_inputs(const inputs& inputs, size_t height);
    void pop_outputs(const outputs& outputs, size_t height);
    code verify(const chain::block& block, size_t height);

    // Asynchronous writers.
    // ------------------------------------------------------------------------

    void do_push_next(const code& ec, block_const_ptr_list_const_ptr blocks,
        size_t index, size_t height, dispatcher& dispatch,
        result_handler handler);
    void do_push(block_const_ptr block, size_t height, dispatcher& dispatch,
        result_handler handler);
    void do_push_block(block_const_ptr block, size_t height,
        dispatcher& dispatch, result_handler handler);
    void do_push_transactions(block_const_ptr block, size_t height,
        size_t bucket, size_t buckets, result_handler handler);
    void handle_push_complete(const code& ec, block_const_ptr block,
        size_t height, result_handler handler);

    std::atomic<bool> closed_;
    const settings& settings_;

    // Cross-database mutext to prevent concurrent file remapping.
    std::shared_ptr<shared_mutex> mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
