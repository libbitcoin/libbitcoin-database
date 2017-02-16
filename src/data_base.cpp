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
using namespace bc::config;
using namespace bc::wallet;

#define NAME "data_base"

// A failure after begin_write is returned without calling end_write.
// This purposely leaves the local flush lock (as enabled) and inverts the
// sequence lock. The former prevents usagage after restart and the latter
// prevents continuation of reads and writes while running. Ideally we
// want to exit without clearing the global flush lock (as enabled).

// Construct.
// ----------------------------------------------------------------------------

data_base::data_base(const settings& settings)
  : closed_(true),
    settings_(settings),
    remap_mutex_(std::make_shared<shared_mutex>()),
    store(settings.directory, settings.index_start_height < without_indexes,
        settings.flush_writes)
{
    LOG_DEBUG(LOG_DATABASE)
        << "Buckets: "
        << "block [" << settings.block_table_buckets << "], "
        << "transaction [" << settings.transaction_table_buckets << "], "
        << "spend [" << settings.spend_table_buckets << "], "
        << "history [" << settings.history_table_buckets << "]";
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
            spends_->create() &&
            history_->create() &&
            stealth_->create();

    if (!created)
        return false;

    // Store the first block.
    push(genesis, 0);
    closed_ = false;
    return true;
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
            spends_->open() &&
            history_->open() &&
            stealth_->open();

    closed_ = false;
    return opened;
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
            spends_->close() &&
            history_->close() &&
            stealth_->close();

    return closed && store::close();
    // Unlock exclusive file access and conditionally the global flush lock.
    ///////////////////////////////////////////////////////////////////////////
}

// protected
void data_base::start()
{
    // TODO: parameterize initial file sizes as record count or slab bytes?

    blocks_ = std::make_shared<block_database>(block_table, block_index,
        settings_.block_table_buckets, settings_.file_growth_rate,
        remap_mutex_);

    transactions_ = std::make_shared<transaction_database>(transaction_table,
        settings_.transaction_table_buckets, settings_.file_growth_rate,
        settings_.cache_capacity, remap_mutex_);

    if (use_indexes)
    {
        spends_ = std::make_shared<spend_database>(spend_table,
            settings_.spend_table_buckets, settings_.file_growth_rate,
            remap_mutex_);

        history_ = std::make_shared<history_database>(history_table,
            history_rows, settings_.history_table_buckets,
            settings_.file_growth_rate, remap_mutex_);

        stealth_ = std::make_shared<stealth_database>(stealth_rows,
            settings_.file_growth_rate, remap_mutex_);
    }
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
            spends_->flush() &&
            history_->flush() &&
            stealth_->flush();

    return flushed;
}

// protected
void data_base::synchronize()
{
    if (use_indexes)
    {
        spends_->synchronize();
        history_->synchronize();
        stealth_->synchronize();
    }

    transactions_->synchronize();
    blocks_->synchronize();
}

// Readers.
// ----------------------------------------------------------------------------

const block_database& data_base::blocks() const
{
    return *blocks_;
}

const transaction_database& data_base::transactions() const
{
    return *transactions_;
}

// Invalid if indexes not initialized.
const spend_database& data_base::spends() const
{
    return *spends_;
}

// Invalid if indexes not initialized.
const history_database& data_base::history() const
{
    return *history_;
}

// Invalid if indexes not initialized.
const stealth_database& data_base::stealth() const
{
    return *stealth_;
}

// Synchronous writers.
// ----------------------------------------------------------------------------

static inline size_t get_next_height(const block_database& blocks)
{
    size_t current_height;
    const auto empty_chain = !blocks.top(current_height);
    return empty_chain ? 0 : current_height + 1;
}

static inline hash_digest get_previous_hash(const block_database& blocks,
    size_t height)
{
    return height == 0 ? null_hash : blocks.get(height - 1).header().hash();
}

code data_base::verify_insert(const block& block, size_t height)
{
    if (block.transactions().empty())
        return error::empty_block;

    if (blocks_->exists(height))
        return error::store_block_duplicate;

    return error::success;
}

code data_base::verify_push(const block& block, size_t height)
{
    if (block.transactions().empty())
        return error::empty_block;

    if (get_next_height(blocks()) != height)
        return error::store_block_invalid_height;

    if (block.header().previous_block_hash() !=
        get_previous_hash(blocks(), height))
        return error::store_block_missing_parent;

    return error::success;
}

code data_base::verify_push(const transaction& tx)
{
    const auto result = transactions_->get(tx.hash(), max_size_t, false);
    return result && !result.is_spent(max_size_t) ? error::unspent_duplicate :
        error::success;
}

bool data_base::begin_insert() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    write_mutex_.lock();

    return begin_write();
}

bool data_base::end_insert() const
{
    write_mutex_.unlock();
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////

    return end_write();
}

// Add block to the database at the given height (gaps allowed/created).
// This is designed for write concurrency but only with itself.
code data_base::insert(const chain::block& block, size_t height)
{
    const auto ec = verify_insert(block, height);

    if (ec)
        return ec;

    if (!push_transactions(block, height) || !push_heights(block, height))
        return error::operation_failed;

    blocks_->store(block, height);
    synchronize();
    return error::success;
}

// This is designed for write exclusivity and read concurrency.
code data_base::push(const chain::transaction& tx, uint32_t forks)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    // Returns error::unspent_duplicate if an unspent tx with same hash exists.
    const auto ec = verify_push(tx);

    if (ec)
        return ec;

    // Begin Flush Lock and Sequential Lock
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::operation_failed;

    // When position is unconfirmed, height is used to store validation forks.
    transactions_->store(tx, forks, transaction_database::unconfirmed);
    transactions_->synchronize();

    return end_write() ? error::success : error::operation_failed;
    // End Sequential Lock and Flush Lock
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Add a block in order (creates no gaps, must be at top).
// This is designed for write exclusivity and read concurrency.
code data_base::push(const block& block, size_t height)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    const auto ec = verify_push(block, height);

    if (ec)
        return ec;

    // Begin Flush Lock and Sequential Lock
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::operation_failed;

    if (!push_transactions(block, height) || !push_heights(block, height))
        return error::operation_failed;

    blocks_->store(block, height);
    synchronize();

    return end_write() ? error::success : error::operation_failed;
    // End Sequential Lock and Flush Lock
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// To push in order call with bucket = 0 and buckets = 1 (defaults).
bool data_base::push_transactions(const chain::block& block, size_t height,
    size_t bucket, size_t buckets)
{
    BITCOIN_ASSERT(bucket < buckets);
    const auto& txs = block.transactions();
    const auto count = txs.size();

    for (auto position = bucket; position < count;
        position = ceiling_add(position, buckets))
    {
        const auto& tx = txs[position];
        transactions_->store(tx, height, position);

        if (height < settings_.index_start_height)
            continue;

        const auto tx_hash = tx.hash();

        if (position != 0)
            push_inputs(tx_hash, height, tx.inputs());

        push_outputs(tx_hash, height, tx.outputs());
        push_stealth(tx_hash, height, tx.outputs());
    }

    return true;
}

bool data_base::push_heights(const chain::block& block, size_t height)
{
    transactions_->synchronize();
    const auto& txs = block.transactions();

    // Skip coinbase as it has no previous output.
    for (auto tx = txs.begin() + 1; tx != txs.end(); ++tx)
        for (const auto& input: tx->inputs())
            if (!transactions_->spend(input.previous_output(), height))
                return false;

    return true;
}

void data_base::push_inputs(const hash_digest& tx_hash, size_t height,
    const input::list& inputs)
{
    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        const auto& input = inputs[index];
        const input_point point{ tx_hash, index };
        spends_->store(input.previous_output(), point);

        // Try to extract an address.
        const auto address = input.address();
        if (!address)
            continue;

        const auto& previous = input.previous_output();
        history_->add_input(address.hash(), point, height, previous);
    }
}

void data_base::push_outputs(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto& output = outputs[index];

        // Try to extract an address.
        const auto address = output.address();
        if (!address)
            continue;

        const auto value = output.value();
        const output_point point{ tx_hash, index };
        history_->add_output(address.hash(), point, height, value);
    }
}

void data_base::push_stealth(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (outputs.empty())
        return;

    // Stealth outputs are paired by convention.
    for (size_t index = 0; index < (outputs.size() - 1); ++index)
    {
        const auto& ephemeral_script = outputs[index].script();
        const auto& payment_output = outputs[index + 1];

        // Try to extract the payment address from the second output.
        const auto address = payment_output.address();
        if (!address)
            continue;

        // Try to extract an unsigned ephemeral key from the first output.
        hash_digest unsigned_ephemeral_key;
        if (!extract_ephemeral_key(unsigned_ephemeral_key, ephemeral_script))
            continue;

        // Try to extract a stealth prefix from the first output.
        uint32_t prefix;
        if (!to_stealth_prefix(prefix, ephemeral_script))
            continue;

        // The payment address versions are arbitrary and unused here.
        const stealth_compact row
        {
            unsigned_ephemeral_key,
            address.hash(),
            tx_hash
        };

        stealth_->store(prefix, height, row);
    }
}

// A false return implies store corruption.
bool data_base::pop(block& out_block)
{
    size_t height;

    // The blockchain is empty (nothing to pop, not even genesis).
    if (!blocks_->top(height))
        return false;

    // This should never become invalid if this call is protected.
    const auto block = blocks_->get(height);
    if (!block)
        return false;

    const auto count = block.transaction_count();
    transaction::list transactions;
    transactions.reserve(count);

    for (size_t position = 0; position < count; ++position)
    {
        const auto tx_hash = block.transaction_hash(position);
        const auto tx = transactions_->get(tx_hash, height, true);

        if (!tx || (tx.height() != height) || (tx.position() != position))
            return false;

        // Deserialize transaction and move it to the block.
        transactions.emplace_back(tx.transaction());
    }

    // Loop txs backwards, the reverse of how they were added.
    // Remove txs, then outputs, then inputs (also reverse order).
    for (auto tx = transactions.rbegin(); tx != transactions.rend(); ++tx)
    {
        if (!transactions_->unconfirm(tx->hash()))
            return false;

        if (!pop_outputs(tx->outputs(), height))
            return false;

        if (!tx->is_coinbase() && !pop_inputs(tx->inputs(), height))
            return false;
    }

    if (!blocks_->unlink(height))
        return false;

    // Synchronise everything that was changed.
    synchronize();

    // Return the block.
    out_block = chain::block(block.header(), std::move(transactions));
    return true;
}

// A false return implies store corruption.
bool data_base::pop_inputs(const input::list& inputs, size_t height)
{
    // Loop in reverse.
    for (auto input = inputs.rbegin(); input != inputs.rend(); ++input)
    {
        if (!transactions_->unspend(input->previous_output()))
            return false;

        if (height < settings_.index_start_height)
            continue;

        // All spends are confirmed.
        if (!spends_->unlink(input->previous_output()))
            return false;

        // Try to extract an address.
        const auto address = payment_address::extract(input->script());

        // All history entries are confirmed.
        if (address && !history_->delete_last_row(address.hash()))
            return false;
    }

    return true;
}

// A false return implies store corruption.
bool data_base::pop_outputs(const output::list& outputs, size_t height)
{
    if (height < settings_.index_start_height)
        return true;

    // Loop in reverse.
    for (auto output = outputs.rbegin(); output != outputs.rend(); ++output)
    {
        // Try to extract an address.
        const auto address = payment_address::extract(output->script());

        // All history entries are confirmed.
        if (address && !history_->delete_last_row(address.hash()))
            return false;

        // All stealth entries are confirmed.
        // Stealth unlink is not implemented as there is no way to correlate.
        ////if (!stealth_->unlink())
        ////    return false;
    }

    return true;
}

// Asynchronous writers.
// ----------------------------------------------------------------------------
// Add a list of blocks in order.
// If the dispatch threadpool is shut down when this is running the handler
// will never be invoked, resulting in a threadpool.join indefinite hang.
void data_base::push_all(block_const_ptr_list_const_ptr in_blocks,
    size_t first_height, dispatcher& dispatch, result_handler handler)
{
    DEBUG_ONLY(safe_add(in_blocks->size(), first_height));

    // This is the beginning of the push_all sequence.
    push_next(error::success, in_blocks, 0, first_height, dispatch, handler);
}

void data_base::push_next(const code& ec,
    block_const_ptr_list_const_ptr blocks, size_t index, size_t height,
    dispatcher& dispatch, result_handler handler)
{
    if (ec || index >= blocks->size())
    {
        // This ends the loop.
        handler(ec);
        return;
    }

    const auto block = (*blocks)[index];

    // Set push start time for the block.
    block->validation.start_push = asio::steady_clock::now();

    const result_handler next =
        std::bind(&data_base::push_next,
            this, _1, blocks, index + 1, height + 1, std::ref(dispatch),
                handler);

    // This is the beginning of the block sub-sequence.
    dispatch.concurrent(&data_base::do_push,
        this, block, height, std::ref(dispatch), next);
}

void data_base::do_push(block_const_ptr block, size_t height,
    dispatcher& dispatch, result_handler handler)
{
    result_handler block_complete =
        std::bind(&data_base::handle_push_transactions,
            this, _1, block, height, handler);

    // This ensures linkage and that the there is at least one tx.
    const auto ec = verify_push(*block, height);

    if (ec)
    {
        block_complete(ec);
        return;
    }

    const auto threads = dispatch.size();
    const auto buckets = std::min(threads, block->transactions().size());
    const auto join_handler = bc::synchronize(std::move(block_complete),
        buckets, NAME "_do_push");

    for (size_t bucket = 0; bucket < buckets; ++bucket)
        dispatch.concurrent(&data_base::do_push_transactions,
            this, block, height, bucket, buckets, join_handler);
}

void data_base::do_push_transactions(block_const_ptr block, size_t height,
    size_t bucket, size_t buckets, result_handler handler)
{
    const auto result = push_transactions(*block, height, bucket, buckets);
    handler(result ? error::success : error::operation_failed);
}

void data_base::handle_push_transactions(const code& ec, block_const_ptr block,
    size_t height, result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    if (!push_heights(*block, height))
    {
        handler(error::operation_failed);
        return;
    }

    // Push the block header and synchronize to complete the block.
    blocks_->store(*block, height);

    // Synchronize tx updates, indexes and block.
    synchronize();

    // Set push end time for the block.
    block->validation.end_push = asio::steady_clock::now();

    // This is the end of the block sub-sequence.
    handler(error::success);
}

// TODO: make async and concurrency as appropriate.
// This precludes popping the genesis block.
void data_base::pop_above(block_const_ptr_list_ptr out_blocks,
    const hash_digest& fork_hash, dispatcher&, result_handler handler)
{
    size_t top;
    out_blocks->clear();

    const auto result = blocks_->get(fork_hash);

    // The fork point does not exist or failed to get it or the top, fail.
    if (!result || !blocks_->top(top))
    {
        handler(error::operation_failed);
        return;
    }

    const auto fork = result.height();
    const auto size = top - fork;

    // The fork is at the top of the chain, nothing to pop.
    if (size == 0)
    {
        handler(error::success);
        return;
    }

    // If the fork is at the top there is one block to pop, and so on.
    out_blocks->reserve(size);

    // Enqueue blocks so .front() is fork + 1 and .back() is top.
    for (size_t height = top; height > fork; --height)
    {
        message::block next;
        const auto start_time = asio::steady_clock::now();

        // TODO: parallelize pop of transactions within each block.
        if (!pop(next))
        {
            handler(error::operation_failed);
            return;
        }

        BITCOIN_ASSERT(next.is_valid());
        auto block = std::make_shared<const message::block>(std::move(next));

        // Mark the blocks as validated for their respective heights.
        block->header().validation.height = height;
        block->validation.error = error::success;
        block->validation.start_pop = start_time;

        // TODO: optimize.
        out_blocks->insert(out_blocks->begin(), block);
    }

    handler(error::success);
}

// This is designed for write exclusivity and read concurrency.
void data_base::reorganize(const checkpoint& fork_point,
    block_const_ptr_list_const_ptr incoming_blocks,
    block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch,
    result_handler handler)
{
    const auto next_height = safe_add(fork_point.height(), size_t(1));

    const result_handler pop_handler =
        std::bind(&data_base::handle_pop,
            this, _1, incoming_blocks, next_height, std::ref(dispatch),
                handler);

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    write_mutex_.lock();

    // Begin Flush Lock and Sequential Lock
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
    {
        pop_handler(error::operation_failed);
        return;
    }

    pop_above(outgoing_blocks, fork_point.hash(), dispatch, pop_handler);
}

void data_base::handle_pop(const code& ec,
    block_const_ptr_list_const_ptr incoming_blocks,
    size_t first_height, dispatcher& dispatch, result_handler handler)
{
    const result_handler push_handler =
        std::bind(&data_base::handle_push,
            this, _1, handler);

    if (ec)
    {
        push_handler(ec);
        return;
    }

    push_all(incoming_blocks, first_height, std::ref(dispatch), push_handler);
}

// We never invoke the caller's handler under the mutex, we never fail to clear
// the mutex, and we always invoke the caller's handler exactly once.
void data_base::handle_push(const code& ec, result_handler handler) const
{
    write_mutex_.unlock();
    // End Critical Section.
    ///////////////////////////////////////////////////////////////////////////

    if (ec)
    {
        handler(ec);
        return;
    }

    handler(end_write() ? error::success : error::operation_failed);
    // End Sequential Lock and Flush Lock
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
}

} // namespace data_base
} // namespace libbitcoin
