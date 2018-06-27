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

// TODO: tx median_time_past must be updated following block metadata.
static constexpr uint32_t no_time = 0;
static constexpr uint32_t genesis_height = 0;
static const auto pool = transaction_state::pooled;
static const auto unverified = machine::rule_fork::unverified;
static const auto unconfirmed = transaction_result::unconfirmed;

// Construct.
// ----------------------------------------------------------------------------

data_base::data_base(const settings& settings)
  : closed_(true),
    settings_(settings),
    database::store(settings.directory, settings.index_addresses,
        settings.flush_writes)
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
    auto created = blocks_->create() && transactions_->create();

    if (settings_.index_addresses)
        created &= addresses_->create();

    created &= push_genesis(genesis) == error::success;

    if (!created)
        return false;

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

    auto opened = blocks_->open() && transactions_->open();

    if (settings_.index_addresses)
        opened &= addresses_->open();

    if (!opened)
        return false;

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

    if (settings_.index_addresses)
    {
        addresses_ = std::make_shared<address_database>(address_table,
            address_rows, settings_.address_table_buckets,
            settings_.file_growth_rate);
    }
}

// protected
void data_base::commit()
{
    if (settings_.index_addresses)
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

    auto flushed = blocks_->flush() && transactions_->flush();

    if (settings_.index_addresses)
        flushed &= addresses_->flush();

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

    auto closed = blocks_->close() && transactions_->close();

    if (settings_.index_addresses)
        closed &= addresses_->close();

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
code data_base::store(const transaction& tx, uint32_t forks)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    // Returns error::unspent_duplicate if an unspent tx with same hash exists.
    if ((ec = verify_push(tx)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // When position is unconfirmed, height is used to store validation forks.
    transactions_->store(tx, forks, no_time, unconfirmed, pool);
    transactions_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// TODO: enable promotion from any unconfirmed state (to confirmed).
// TODO: otherwise this will replace the previously-existing header.
// This expects header is validated and not yet stored.
code data_base::push(const chain::header& header, size_t height)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_push(header, height)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // State is indexed | pent (pending download).
    blocks_->push(header, height);
    blocks_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// This expects header exists at the top of the header index.
code data_base::pop(chain::header& out_header, size_t height)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((verify_top(height, false)))
        return ec;

    const auto result = blocks_->get(height, false);

    if (!result)
        return error::operation_failed;

    ////// Create a block for walking transactions.
    ////const chain::block block(result.header(), to_transactions(result));

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    ////if ((ec = pop_transactions(block)))
    ////    return ec;

    if (!blocks_->unconfirm(result.hash(), height, false))
        return error::operation_failed;

    // Commit everything that was changed and return header.
    blocks_->commit();
    out_header = result.header();
    BITCOIN_ASSERT(out_header.is_valid());

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
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    if ((ec = verify_update(*block, height)))
        return ec;

    // Conditional write mutex preserves write flushing by preventing overlap.
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    for (const auto& tx: block->transactions())
    {
        // This stores the transaction and sets tx link metadata.
        if (!transactions_->store(tx, unverified, no_time, unconfirmed, pool))
            return error::operation_failed;

        ////// HACK: added this temporarily, to measure tx store efficiency.
        ////// This causes out-of-order indexing, which would break reorganization.
        ////// This also implies indexing of inputs without prevouts, so imperfect.
        ////// With validation we have prevouts and can control order, though not
        ////// as fast and requires full validation (for all prevouts) or short
        ////// validation (checkpoint/milestone) population of prevouts.
        ////if (settings_.index_addresses)
        ////{
        ////    push_inputs(tx);
        ////    push_outputs(tx);
        ////}
    }

    // Update the block's transaction associations (not its state).
    if (!blocks_->update(*block))
        return error::operation_failed;

    commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

code data_base::validate(block_const_ptr block, bool positive)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    if ((ec = verify(*block)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    if (!blocks_->validate(block->hash(), positive))
        return error::operation_failed;

    return end_write() ? error::success : error::store_lock_failure;
    ///////////////////////////////////////////////////////////////////////////
}

// private
// Store, update, validate and confirm the genesis block.
code data_base::push_genesis(const block& block)
{
    auto ec = push(block.header(), genesis_height);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_update(block, genesis_height)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Pushes transactions sequentially as **confirmed**.
    if ((ec = push_transactions(block, genesis_height, no_time)))
        return ec;

    // Populate pent block's transaction references.
    blocks_->update(block);

    // Promote validation state from pent to **valid**.
    blocks_->validate(block.hash(), true);

    // Promote index state from indexed to **confirmed**.
    blocks_->confirm(block.hash(), genesis_height, true);
    commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Debug Utilities.
// ----------------------------------------------------------------------------
// protected

#ifndef NDEBUG
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
#endif

code data_base::verify(const block& block) const
{
#ifdef NDEBUG
    if (!blocks_->get(block.hash()))
        return error::not_found;
#endif

    return error::success;
}

code data_base::verify(const config::checkpoint& fork_point,
    bool block_index) const
{
#ifndef NDEBUG
    const auto result = blocks_->get(fork_point.hash());

    if (!result)
        return error::not_found;

    if (fork_point.height() != result.height())
        return error::store_block_invalid_height;

    const auto state = result.state();

    if (!is_confirmed(state) && (block_index || !is_indexed(state)))
        return error::store_incorrect_state;
#endif

    return error::success;
}

code data_base::verify_top(size_t height, bool block_index) const
{
#ifndef NDEBUG
    size_t actual_height;
    if (!blocks_->top(actual_height, block_index)
        || !(actual_height == height))
        return error::operation_failed;
#endif

    return error::success;
}


code data_base::verify_push(const transaction& tx) const
{
#ifndef NDEBUG
    const auto result = transactions_->get(tx.hash());

    // This is an expensive re-check, but only if a duplicate exists.
    if (result && !result.is_spent())
        return error::unspent_duplicate;
#endif

    return error::success;
}

code data_base::verify_push(const header& header, size_t height) const
{
#ifndef NDEBUG
    if (get_next_block(blocks(), false) != height)
        return error::store_block_invalid_height;

    if (get_previous_block(blocks(), height, false) !=
        header.previous_block_hash())
        return error::store_block_missing_parent;
#endif

    return error::success;
}

code data_base::verify_push(const block& block, size_t height) const
{
    if (block.transactions().empty())
        return error::empty_block;

#ifndef NDEBUG
    if (get_next_block(blocks(), true) != height)
        return error::store_block_invalid_height;

    if (get_previous_block(blocks(), height, true) !=
        block.header().previous_block_hash())
        return error::store_block_missing_parent;
#endif

    return error::success;
}

code data_base::verify_update(const block& block, size_t height) const
{
#ifndef NDEBUG
    if (block.transactions().empty())
        return error::empty_block;

    if (get_block(blocks(), height, false) != block.hash())
        return error::not_found;
#endif

    return error::success;
}

// Synchronous transaction writers.
// ----------------------------------------------------------------------------
// protected

// To push in order call with bucket = 0 and buckets = 1 (defaults).
code data_base::push_transactions(const block& block, size_t height,
    uint32_t median_time_past, size_t bucket, size_t buckets,
    transaction_state )
{
    BITCOIN_ASSERT(bucket < buckets);
    const auto& txs = block.transactions();
    const auto count = txs.size();

    for (auto position = bucket; position < count;
        position = ceiling_add(position, buckets))
    {
        const auto& tx = txs[position];

        // This stores and confirms the transaction, and sets tx link metadata.
        if (!transactions_->store(tx, height, median_time_past, position))
            return error::operation_failed;

        if (settings_.index_addresses)
        {
            push_inputs(tx);
            push_outputs(tx);
        }
    }

    return error::success;
}

// TODO: add segwit address indexing.
void data_base::push_inputs(const transaction& tx)
{
    if (tx.is_coinbase())
        return;

    uint32_t index = 0;
    const auto& inputs = tx.inputs();
    const auto link = tx.metadata.link;

    for (const auto& input: inputs)
    {
        const auto& prevout = input.previous_output();
        const payment_record in{ link, index++, prevout.checksum(), false };

        if (prevout.metadata.cache.is_valid())
        {
            // This results in a complete and unambiguous history for the
            // address since standard outputs contain unambiguous address data.
            for (const auto& address: prevout.metadata.cache.addresses())
                addresses_->store(address.hash(), in);
        }
        else
        {
            // For any p2pk spend this creates no record (insufficient data).
            // For any p2kh spend this creates the ambiguous p2sh address,
            // which significantly expands the size of the history store.
            // These are tradeoffs when no prevout is cached (checkpoint sync).
            for (const auto& address: input.addresses())
                addresses_->store(address.hash(), in);
        }
    }
}

// TODO: add segwit address indexing.
void data_base::push_outputs(const transaction& tx)
{
    uint32_t index = 0;
    const auto& outputs = tx.outputs();
    const auto link = tx.metadata.link;

    for (const auto& output: outputs)
    {
        const payment_record out{ link, index++, output.value(), true };

        // Standard outputs contain unambiguous address data.
        for (const auto& address: output.addresses())
            addresses_->store(address.hash(), out);
    }
}

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

        if (!transactions_->unconfirm(tx.metadata.link))
            return error::operation_failed;

        if (settings_.index_addresses)
            if (!pop_inputs(tx) || !pop_outputs(tx))
                return error::operation_failed;
    }

    return error::success;
}

bool data_base::pop_inputs(const chain::transaction& tx)
{
    if (!settings_.index_addresses || tx.is_coinbase())
        return true;

    for (auto input: tx.inputs())
        for (const auto& address: input.addresses())
            if (!addresses_->pop(address.hash()))
                return false;

    return true;
}

bool data_base::pop_outputs(const chain::transaction& tx)
{
    if (!settings_.index_addresses)
        return true;

    for (auto output: tx.outputs())
        for (const auto& address: output.addresses())
            if (!addresses_->pop(address.hash()))
                return false;

    return true;
}

// Header reorganization.
// ----------------------------------------------------------------------------

code data_base::reorganize(const config::checkpoint& fork_point,
    header_const_ptr_list_const_ptr incoming,
    header_const_ptr_list_ptr outgoing)
{
    if (fork_point.height() > max_size_t - incoming->size())
        return error::operation_failed;

    const auto result =
        pop_above(outgoing, fork_point) &&
        push_all(incoming, fork_point);

    return result ? error::success : error::operation_failed;
}

// private
bool data_base::pop_above(header_const_ptr_list_ptr headers,
    const config::checkpoint& fork_point)
{
    code ec;
    headers->clear();
    if ((ec = verify(fork_point, false)))
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
    }

    return true;
}

// private
bool data_base::push_all(header_const_ptr_list_const_ptr headers,
    const config::checkpoint& fork_point)
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

// Utility writers.
// ----------------------------------------------------------------------------
// These are not used by the node.

// TODO: enable promotion from any unconfirmed state (to indexed).
// TODO: otherwise this will replace the previously-existing block.
// This expects block is validated, header is not yet stored.
code data_base::push(const block& block, size_t height,
    uint32_t median_time_past)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_push(block, height)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Pushes transactions sequentially as confirmed.
    if ((ec = push_transactions(block, height, median_time_past)))
        return ec;

    blocks_->push(block, height, median_time_past);
    commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// This expects block exists at the top of the block index.
code data_base::pop(chain::block& out_block, size_t height)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_top(height, true)))
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

// private
transaction::list data_base::to_transactions(const block_result& result) const
{
    transaction::list txs;
    txs.reserve(result.transaction_count());

    for (const auto link: result)
    {
        const auto tx = transactions_->get(link);
        BITCOIN_ASSERT(tx);
        txs.push_back(tx.transaction());
    }

    return txs;
}

} // namespace database
} // namespace libbitcoin
