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
#include <bitcoin/database/data_base.hpp>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;
using namespace bc::wallet;

// Construct.
// ----------------------------------------------------------------------------

data_base::data_base(const settings& settings)
  : closed_(true),
    settings_(settings),
    mutex_(std::make_shared<shared_mutex>()),
    store(settings.directory, settings.index_start_height < without_indexes)
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
        created &=
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
    // Lock exclusive file access.
    if (!store::open())
        return false;

    start();

    auto opened = 
        blocks_->open() &&
        transactions_->open();
    
    if (use_indexes)
        opened &=
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
        closed &=
            spends_->close() &&
            history_->close() &&
            stealth_->close();

    return closed && store::close();
    // Unlocked exclusive file access.
    ///////////////////////////////////////////////////////////////////////////
}

// protected
void data_base::start()
{
    // TODO: parameterize initial file sizes as record count or slab bytes?

    blocks_ = std::make_shared<block_database>(block_table, block_index,
        settings_.block_table_buckets, settings_.file_growth_rate, mutex_);

    transactions_ = std::make_shared<transaction_database>(transaction_table,
        settings_.transaction_table_buckets, settings_.file_growth_rate,
        mutex_);

    if (use_indexes)
    {
        spends_ = std::make_shared<spend_database>(spend_table,
            settings_.spend_table_buckets, settings_.file_growth_rate,
            mutex_);

        history_ = std::make_shared<history_database>(history_table,
            history_rows, settings_.history_table_buckets,
            settings_.file_growth_rate, mutex_);

        stealth_ = std::make_shared<stealth_database>(stealth_rows,
            settings_.file_growth_rate, mutex_);
    }
}

// protected
bool data_base::flush()
{
    if (closed_)
        return true;

    auto flushed =
        blocks_->flush() &&
        transactions_->flush();

    if (use_indexes)
        flushed &=
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

// Writers.
// ----------------------------------------------------------------------------

static size_t get_next_height(const block_database& blocks)
{
    size_t current_height;
    const auto empty_chain = !blocks.top(current_height);
    return empty_chain ? 0 : current_height + 1;
}

static hash_digest get_previous_hash(const block_database& blocks,
    size_t height)
{
    return height == 0 ? null_hash : blocks.get(height - 1).header().hash();
}

// Add block to the database at the given height.
bool data_base::insert(const chain::block& block, size_t height)
{
    if (blocks_->exists(height))
        return false;

    push_transactions(block, height);
    blocks_->store(block, height);
    synchronize();
    return true;
}

// Add a list of blocks in order.
bool data_base::push(const block::list& blocks, size_t first_height)
{
    auto height = first_height;

    for (const auto block: blocks)
        if (!push(block, height++))
            return false;

    return true;
}

// Add a block in order.
bool data_base::push(const block& block, size_t height)
{
    if (get_next_height(blocks()) != height)
    {
        LOG_ERROR(LOG_DATABASE)
            << "The block is out of order at height [" << height << "].";
        return false;
    }

    if (block.header().previous_block_hash() != 
        get_previous_hash(blocks(), height))
    {
        LOG_ERROR(LOG_DATABASE)
            << "The block has incorrect parent for height [" << height << "].";
        return false;
    }

    push_transactions(block, height);
    blocks_->store(block, height);
    synchronize();
    return true;
}

// Add transactions and related indexing for a given block.
void data_base::push_transactions(const block& block, size_t height)
{
    for (size_t index = 0; index < block.transactions().size(); ++index)
    {
        const auto& tx = block.transactions()[index];
        const auto tx_hash = tx.hash();

        // Add inputs
        if (!tx.is_coinbase())
            push_inputs(tx_hash, height, tx.inputs());

        // Add outputs
        push_outputs(tx_hash, height, tx.outputs());

        // Add stealth outputs
        push_stealth(tx_hash, height, tx.outputs());

        // Add transaction
        transactions_->store(height, index, tx);
    }
}

void data_base::push_inputs(const hash_digest& tx_hash, size_t height,
    const input::list& inputs)
{
    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        const auto& input = inputs[index];
        const input_point point{ tx_hash, index };

        /* bool */ transactions_->update(input.previous_output(), height);

        if (height < settings_.index_start_height)
            continue;

        spends_->store(input.previous_output(), point);

        // Try to extract an address.
        const auto address = payment_address::extract(input.script());
        if (!address)
            continue;

        const auto& previous = input.previous_output();
        history_->add_input(address.hash(), point, height, previous);
    }
}

void data_base::push_outputs(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (height < settings_.index_start_height)
        return;

    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto& output = outputs[index];
        const output_point point{ tx_hash, index };

        // Try to extract an address.
        const auto address = payment_address::extract(output.script());
        if (!address)
            continue;

        const auto value = output.value();
        history_->add_output(address.hash(), point, height, value);
    }
}

void data_base::push_stealth(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (height < settings_.index_start_height || outputs.empty())
        return;

    // Stealth outputs are paired by convention.
    for (size_t index = 0; index < (outputs.size() - 1); ++index)
    {
        const auto& ephemeral_script = outputs[index].script();
        const auto& payment_script = outputs[index + 1].script();

        // Try to extract an unsigned ephemeral key from the first output.
        hash_digest unsigned_ephemeral_key;
        if (!extract_ephemeral_key(unsigned_ephemeral_key, ephemeral_script))
            continue;

        // Try to extract a stealth prefix from the first output.
        uint32_t prefix;
        if (!to_stealth_prefix(prefix, ephemeral_script))
            continue;

        // Try to extract the payment address from the second output.
        const auto address = payment_address::extract(payment_script);
        if (!address)
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

// This precludes popping the genesis block.
// Returns true with empty list if for is at the top.
bool data_base::pop_above(block::list& out_blocks,
    const hash_digest& fork_hash)
{
    size_t top;
    out_blocks.clear();
    const auto result = blocks_->get(fork_hash);

    // The fork point does not exist or failed to get it or the top, fail.
    if (!result || !blocks_->top(top))
        return false;

    const auto fork = result.height();
    const auto size = top - fork;

    // The fork is at the top of the chain, nothing to pop.
    if (size == 0)
        return true;

    // If the fork is at the top there is one block to pop, and so on.
    out_blocks.reserve(size);

    // Enqueue blocks so .front() is fork + 1 and .back() is top.
    for (size_t height = top; height > fork; --height)
    {
        // DON'T MAKE BLOCK CONST, INVALIDATES THE MOVE.
        auto block = pop();

        if (!block.is_valid())
            return false;

        // Mark the blocks as validated for their respective heights.
        block.header().validation.height = height;
        block.validation.result = error::success;

        // Move the block as an r-value into the list (no copy).
        out_blocks.insert(out_blocks.begin(), std::move(block));
    }

    return true;
}

block data_base::pop()
{
    size_t height;

    // The blockchain is empty (nothing to pop, not even genesis).
    if (!blocks_->top(height))
        return{};

    const auto block_result = blocks_->get(height);

    // The height is invalid (should not happen if locked).
    if (!block_result)
        return{};

    chain::block block;
    auto& txs = block.transactions();

    for (size_t tx = 0; tx < block_result.transaction_count(); ++tx)
    {
        const auto tx_hash = block_result.transaction_hash(tx);

        // We want the highest tx with this hash (allow max height).
        const auto tx_result = transactions_->get(tx_hash, max_size_t);

        if (!tx_result || tx_result.height() != height ||
            tx_result.position() != tx)
            return{};

        // Deserialize transaction and move it to the block.
        txs.emplace_back(tx_result.transaction());
    }

    // Loop txs backwards, the reverse of how they are added.
    // Remove txs, then outputs, then inputs (also reverse order).
    for (auto tx = txs.rbegin(); tx != txs.rend(); ++tx)
    {
        /* bool */ transactions_->unlink(tx->hash());
        pop_outputs(tx->outputs(), height);

        if (!tx->is_coinbase())
            pop_inputs(tx->inputs(), height);
    }

    /* bool */ blocks_->unlink(height);

    // Synchronise everything that was changed.
    synchronize();

    // Return the block.
    return chain::block(block_result.header(), txs);
}

void data_base::pop_inputs(const input::list& inputs, size_t height)
{
    static const auto not_spent = output::validation::not_spent;

    // Loop in reverse.
    for (auto input = inputs.rbegin(); input != inputs.rend(); ++input)
    {
        /* bool */ transactions_->update(input->previous_output(), not_spent);

        if (height < settings_.index_start_height)
            continue;

        /* bool */ spends_->unlink(input->previous_output());

        // Try to extract an address.
        const auto address = payment_address::extract(input->script());

        if (address)
            /* bool */ history_->delete_last_row(address.hash());
    }
}

void data_base::pop_outputs(const output::list& outputs, size_t height)
{
    if (height < settings_.index_start_height)
        return;

    // Loop in reverse.
    for (auto output = outputs.rbegin(); output != outputs.rend(); ++output)
    {
        // Try to extract an address.
        const auto address = payment_address::extract(output->script());

        if (address)
            /* bool */ history_->delete_last_row(address.hash());

        // TODO: try to extract a stealth info and if found unlink index.
        // Stealth unlink is not implemented.
        /* bool */ stealth_->unlink();
    }
}

} // namespace data_base
} // namespace libbitcoin
