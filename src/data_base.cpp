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
#include <bitcoin/database/data_base.hpp>

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>

namespace libbitcoin {
namespace database {

using namespace std::placeholders;
using namespace boost::filesystem;
using namespace bc::chain;
using namespace bc::wallet;

#define NAME "data_base"

// TODO: replace spends with complex query, output gets inpoint:
// (1) transactions_.get(outpoint, require_confirmed)->spender_height.
// (2) blocks_.get(spender_height)->transactions().
// (3) (transactions()->inputs()->previous_output() == outpoint)->inpoint.
// This has the same average cost as 1 output-query + 1/2 block-query.
// This will reduce server indexing by 30% (address indexing only).
// Could make index optional, redirecting queries if not present.

// A failure after begin_write is returned without calling end_write.
// This leaves the local flush lock enabled, preventing usage after restart.

// Construct.
// ----------------------------------------------------------------------------

data_base::data_base(const settings& settings)
  : closed_(true),
    settings_(settings),
    store(settings.directory, settings.index_addresses, settings.flush_writes)
{
    LOG_DEBUG(LOG_DATABASE)
        << "Buckets: "
        << "block [" << settings.block_table_buckets << "], "
        << "transaction [" << settings.transaction_table_buckets << "], "
        << "address [" << settings.address_table_buckets << "]";
}

data_base::~data_base()
{
    close();
}

// Open and close.
// ----------------------------------------------------------------------------

// Throws if there is insufficient disk space, not idempotent.
bool data_base::create(const block& genesis)
{
    ///////////////////////////////////////////////////////////////////////////
    // Lock exclusive file access.
    if (!store::open())
        return false;

    // Create files.
    if (!store::create())
        return false;

    start();

    // These leave the databases open.
    auto created =
        blocks_->create() &&
        transactions_->create();

    if (use_indexes)
        created = created &&
        addresses_->create();

    if (!created)
        return false;

    // Index, populate, validate and confirm the first header/block.
    created = push(genesis.header(), 0) == error::success &&
        update_genesis(genesis) == error::success;

    closed_ = false;
    return created;
}

// Must be called before performing queries, not idempotent.
// May be called after stop and/or after close in order to reopen.
bool data_base::open()
{
    ///////////////////////////////////////////////////////////////////////////
    // Lock exclusive file access and conditionally the global flush lock.
    if (!store::open())
        return false;

    start();

    auto opened =
        blocks_->open() &&
        transactions_->open();

    if (use_indexes)
        opened = opened &&
        addresses_->open();

    closed_ = false;
    return opened;
}

// protected
void data_base::start()
{
    blocks_ = std::make_shared<block_database>(block_table, header_index,
        block_index, transaction_index, settings_.block_table_buckets,
        settings_.file_growth_rate);

    transactions_ = std::make_shared<transaction_database>(transaction_table,
        settings_.transaction_table_buckets, settings_.file_growth_rate,
        settings_.cache_capacity);

    if (use_indexes)
    {
        addresses_ = std::make_shared<address_database>(address_table,
            address_rows, settings_.address_table_buckets,
            settings_.file_growth_rate);
    }
}

// protected
void data_base::commit()
{
    if (use_indexes)
        addresses_->commit();

    transactions_->commit();
    blocks_->commit();
}

// protected
bool data_base::flush() const
{
    // Avoid a race between flush and close whereby flush is skipped because
    // close is true and therefore the flush lock file is deleted before close
    // fails. This would leave the database corrupted and undetected. The flush
    // call must execute and successfully flush or the lock must remain.
    ////if (closed_)
    ////    return true;

    auto flushed =
        blocks_->flush() &&
        transactions_->flush();

    if (use_indexes)
        flushed = flushed &&
        addresses_->flush();

    LOG_DEBUG(LOG_DATABASE)
        << "Write flushed to disk: "
        << code(flushed ? error::success : error::operation_failed).message();

    return flushed;
}

// Close is idempotent and thread safe.
// Optional as the database will close on destruct.
bool data_base::close()
{
    if (closed_)
        return true;

    closed_ = true;

    auto closed =
        blocks_->close() &&
        transactions_->close();

    if (use_indexes)
        closed = closed &&
        addresses_->close();

    return closed && store::close();
    // Unlock exclusive file access and conditionally the global flush lock.
    ///////////////////////////////////////////////////////////////////////////
}

// Reader interfaces.
// ----------------------------------------------------------------------------
// public

const block_database& data_base::blocks() const
{
    return *blocks_;
}

const transaction_database& data_base::transactions() const
{
    return *transactions_;
}

// Invalid if indexes not initialized.
const address_database& data_base::addresses() const
{
    return *addresses_;
}

// Synchronous writers.
// ----------------------------------------------------------------------------
// public

// TODO: enable promotion from any unconfirmed state to pooled.
// This expects tx is validated, unconfirmed and not yet stored.
code data_base::push(const transaction& tx, uint32_t forks)
{
    static const uint32_t median_time_past = 0;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    // Returns error::unspent_duplicate if an unspent tx with same hash exists.
    const auto ec = verify_push(tx);

    if (ec)
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // When position is unconfirmed, height is used to store validation forks.
    transactions_->store(tx, forks, median_time_past,
        transaction_result::unconfirmed, transaction_state::pooled);
    transactions_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// TODO: enable promotion from any unconfirmed state (to confirmed).
// TODO: otherwise this will replace the previously-existing header.
// This expects header is validated and not yet stored.
code data_base::push(const header& header, size_t height)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    const auto ec = verify_push(header, height);

    if (ec)
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // State is indexed | pent.
    blocks_->push(header, height);
    blocks_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// TODO: enable promotion from any unconfirmed state (to indexed).
// TODO: otherwise this will replace the previously-existing block.
// This expects block is validated, header is not yet stored.
code data_base::push(const block& block, size_t height)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    auto ec = verify_push(block, height);

    if (ec)
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    const auto median_time_past = block.header().validation.median_time_past;

    // Pushes transactions sequentially as confirmed.
    if ((ec = push_transactions(block, height, median_time_past)))
        return ec;

    blocks_->push(block, height);
    commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Add transactions for an existing block header.
// This assumes the txs do not exist, which is ok for IBD, but not catch-up.
// This allows parallel write when write flushing is not enabled.
// TODO: optimize for catch-up sync by updating existing transactions.
code data_base::update(block_const_ptr block, size_t height)
{
    // TODO: tx median_time_past must be updated following block validation.
    static constexpr uint32_t median_time_past = 0;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    auto ec = verify_update(*block, height);

    if (ec)
        return ec;

    // Conditional write mutex preserves write flushing by preventing overlap.
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    for (const auto& tx: block->transactions())
    {
        if (!transactions_->store(tx, machine::rule_fork::unverified,
            median_time_past, transaction_result::unconfirmed,
            transaction_state::pooled))
        {
            return error::operation_failed;
        }
    }

    // Update the block's transaction references (not its state).
    blocks_->update(*block);
    commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Update, validate and confirm the genesis block.
code data_base::update_genesis(const block& block)
{
    static constexpr uint32_t height = 0;
    static constexpr uint32_t median_time_past = 0;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    auto ec = verify_update(block, height);

    if (ec)
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Pushes transactions sequentially as **confirmed**.
    if ((ec = push_transactions(block, height, median_time_past)))
        return ec;

    // Populate pent block's transaction references.
    blocks_->update(block);

    // Promote validation state from pent to **valid**.
    blocks_->validate(block.hash(), true);

    // Promote index state from indexed to **confirmed**.
    blocks_->confirm(block.hash(), height, true);
    commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// This expects block exists at the top of the block index.
code data_base::pop(chain::block& out_block, size_t height)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    auto ec = verify_top(height, true);

    if (ec)
        return ec;

    const auto result = blocks_->get(height, true);

    if (!result)
        return error::operation_failed;

    // Create a block for walking transactions and return.
    out_block = chain::block(result.header(), to_transactions(result));

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    if ((ec = pop_transactions(out_block)))
        return ec;

    if (!blocks_->unconfirm(out_block.hash(), height, true))
        return error::operation_failed;

    // Commit everything that was changed.
    commit();

    BITCOIN_ASSERT(out_block.is_valid());
    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// This expects header exists at the top of the header index.
code data_base::pop(chain::header& out_header, size_t height)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    auto ec = verify_top(height, false);

    if (ec)
        return ec;

    const auto result = blocks_->get(height, false);

    if (!result)
        return error::operation_failed;

    // Create a block for walking transactions.
    const chain::block block(result.header(), to_transactions(result));

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    if ((ec = pop_transactions(block)))
        return ec;

    if (!blocks_->unconfirm(block.hash(), height, false))
        return error::operation_failed;

    // Commit everything that was changed.
    commit();

    out_header = block.header();
    BITCOIN_ASSERT(out_header.is_valid());
    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Utilities.
// ----------------------------------------------------------------------------
// protected

static hash_digest get_block(const block_database& blocks,
    size_t height, bool block_index)
{
    return blocks.get(height, block_index).hash();
}

static hash_digest get_previous_block(const block_database& blocks,
    size_t height, bool block_index)
{
    return height == 0 ? null_hash : get_block(blocks, height - 1, block_index);
}

static size_t get_next_block(const block_database& blocks,
    bool block_index)
{
    size_t current_height;
    const auto empty_chain = !blocks.top(current_height, block_index);
    return empty_chain ? 0 : current_height + 1;
}

code data_base::verify_top(size_t height, bool block_index) const
{
    size_t actual_height;
    return blocks_->top(actual_height, block_index) &&
        (actual_height == height) ? error::success : error::operation_failed;
}

code data_base::verify(const checkpoint& fork_point, bool block_index) const
{
    const auto result = blocks_->get(fork_point.hash());
    if (!result)
        return error::not_found;

    if (fork_point.height() != result.height())
        return error::store_block_invalid_height;

    const auto state = result.state();
    if (!is_confirmed(state) && (block_index || !is_indexed(state)))
        return error::store_incorrect_state;

    return error::success;
}

// This store-level check is a failsafe for blockchain behavior.
code data_base::verify_push(const transaction& tx) const
{
    const auto result = transactions_->get(tx.hash());

    // This is an expensive re-check, but only if a duplicate exists.
    if (result && !result.is_spent())
        return error::unspent_duplicate;

    return error::success;
}

// This store-level check is a failsafe for blockchain behavior.
code data_base::verify_push(const header& header, size_t height) const
{
    if (get_next_block(blocks(), false) != height)
        return error::store_block_invalid_height;

    if (get_previous_block(blocks(), height, false) !=
        header.previous_block_hash())
        return error::store_block_missing_parent;

    return error::success;
}

// This store-level check is a failsafe for blockchain behavior.
code data_base::verify_push(const block& block, size_t height) const
{
    if (block.transactions().empty())
        return error::empty_block;

    if (get_next_block(blocks(), true) != height)
        return error::store_block_invalid_height;

    if (get_previous_block(blocks(), height, true) !=
        block.header().previous_block_hash())
        return error::store_block_missing_parent;

    return error::success;
}

// This store-level check is a failsafe for blockchain behavior.
code data_base::verify_update(const block& block, size_t height) const
{
    if (block.transactions().empty())
        return error::empty_block;

    if (get_block(blocks(), height, false) != block.hash())
        return error::not_found;

    return error::success;
}

// TODO: could move into block_result but there is tx store reference.
transaction::list data_base::to_transactions(const block_result& result) const
{
    transaction::list txs;
    txs.reserve(result.transaction_count());

    for (const auto link: result.transaction_links())
    {
        const auto result = transactions_->get(link);
        BITCOIN_ASSERT(static_cast<bool>(result));
        txs.push_back(result.transaction());
        txs.back().validation.link = link;
    }

    return txs;
}

// Synchronous transaction writers.
// ----------------------------------------------------------------------------
// protected

// A false return implies store corruption.
// To push in order call with bucket = 0 and buckets = 1 (defaults).
code data_base::push_transactions(const block& block, size_t height,
    uint32_t median_time_past, size_t bucket, size_t buckets,
    transaction_state state)
{
    BITCOIN_ASSERT(bucket < buckets);
    const auto& txs = block.transactions();
    const auto count = txs.size();

    for (auto position = bucket; position < count;
        position = ceiling_add(position, buckets))
    {
        const auto& tx = txs[position];

        if (!transactions_->store(tx, height, median_time_past, position,
            state))
            return error::operation_failed;

        if (settings_.index_addresses)
        {
            push_inputs(tx, height);
            push_outputs(tx, height);
        }
    }

    return error::success;
}

void data_base::push_inputs(const transaction& tx, size_t height)
{
    if (tx.is_coinbase())
        return;

    const auto hash = tx.hash();
    const auto& inputs = tx.inputs();

    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        const input_point inpoint{ hash, index };
        const auto& input = inputs[index];
        const auto& prevout = input.previous_output();
        const auto checksum = prevout.checksum();

        if (prevout.validation.cache.is_valid())
        {
            // This results in a complete and unambiguous history for the
            // address since standard outputs contain unambiguous address data.
            for (const auto& address: prevout.validation.cache.addresses())
                addresses_->store(address.hash(), { height, inpoint, checksum });
        }
        else
        {
            // For any p2pk spend this creates no record (insufficient data).
            // For any p2kh spend this creates the ambiguous p2sh address,
            // which significantly expands the size of the history store.
            // These are tradeoffs when no prevout is cached (checkpoint sync).
            for (const auto& address: input.addresses())
                addresses_->store(address.hash(), { height, inpoint, checksum });
        }
    }
}

void data_base::push_outputs(const transaction& tx, size_t height)
{
    const auto hash = tx.hash();
    const auto& outputs = tx.outputs();

    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto outpoint = output_point{ hash, index };
        const auto& output = outputs[index];
        const auto value = output.value();
        using script = bc::chain::script;

        // Standard outputs contain unambiguous address data.
        for (const auto& address: output.addresses())
            addresses_->store(address.hash(), { height, outpoint, value });
    }
}

// A false return implies store corruption.
// To pop in order call with bucket = 0 and buckets = 1 (defaults).
code data_base::pop_transactions(const block& block, size_t bucket,
    size_t buckets)
{
    BITCOIN_ASSERT(bucket < buckets);
    const auto& txs = block.transactions();
    const auto count = txs.size();

    for (auto position = bucket; position < count;
        position = ceiling_add(position, buckets))
    {
        const auto& tx = txs[position];

        if (!transactions_->unconfirm(tx))
            return error::operation_failed;

        if (settings_.index_addresses)
        {
            if (!pop_inputs(tx) ||
                !pop_outputs(tx))
                return error::operation_failed;
        }
    }

    return error::success;
}

// A false return implies store corruption.
bool data_base::pop_inputs(const chain::transaction& tx)
{
    if (!settings_.index_addresses || tx.is_coinbase())
        return true;

    const auto& inputs = tx.inputs();

    for (auto input = inputs.begin(); input != inputs.end(); ++input)
        for (const auto& address: input->addresses())
            if (!addresses_->pop(address.hash()))
                return false;

    return true;
}

// A false return implies store corruption.
bool data_base::pop_outputs(const chain::transaction& tx)
{
    if (!settings_.index_addresses)
        return true;

    const auto& outputs = tx.outputs();

    for (auto output = outputs.begin(); output != outputs.end(); ++output)
        for (const auto& address: output->addresses())
            if (!addresses_->pop(address.hash()))
                return false;

    return true;
}

// Header Reorganization (sequential).
// ----------------------------------------------------------------------------

// A false return implies store corruption.
void data_base::reorganize(const config::checkpoint& fork_point,
    header_const_ptr_list_const_ptr incoming,
    header_const_ptr_list_ptr outgoing, dispatcher& dispatch,
    result_handler handler)
{
    if (fork_point.height() > max_size_t - incoming->size())
    {
        handler(error::operation_failed);
        return;
    }

    const auto result =
        pop_above(outgoing, fork_point, dispatch, handler) &&
        push_all(incoming, fork_point, dispatch, handler);

    handler(result ? error::success : error::operation_failed);
}

// A false return implies store corruption.
bool data_base::pop_above(header_const_ptr_list_ptr headers,
    const checkpoint& fork_point, dispatcher& dispatch, result_handler handler)
{
    headers->clear();
    auto ec = verify(fork_point, false);
    if (ec)
        return false;

    size_t top;
    if (!blocks_->top(top, false))
        return false;

    const auto fork = fork_point.height();
    const auto depth = top - fork;
    headers->reserve(depth);

    if (depth == 0)
        return true;

    // Pop all headers above the fork point.
    for (size_t height = top; height > fork; --height)
    {
        const auto next = std::make_shared<message::header>();
        if ((ec = pop(*next, height)))
            return false;

        headers->insert(headers->begin(), next);
        next->validation.height = height;
    }

    return true;
}

// A false return implies store corruption.
bool data_base::push_all(header_const_ptr_list_const_ptr headers,
    const checkpoint& fork_point, dispatcher& dispatch, result_handler handler)
{
    code ec;
    const auto first_height = fork_point.height() + 1;

    // Push all headers onto the fork point.
    for (size_t index = 0; index < headers->size(); ++index)
    {
        const auto next = (*headers)[index];
        if ((ec = push(*next, first_height + index)))
            return false;
    }

    return true;
}

} // namespace database
} // namespace libbitcoin
