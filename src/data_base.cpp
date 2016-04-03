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
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/settings.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;
using namespace bc::wallet;

// BIP30 exception blocks.
// github.com/bitcoin/bips/blob/master/bip-0030.mediawiki#specification
static const config::checkpoint exception1 =
{ "00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec", 91842 };
static const config::checkpoint exception2 =
{ "00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721", 91880 };

bool data_base::touch_file(const path& file_path)
{
    bc::ofstream file(file_path.string());
    if (file.bad())
        return false;

    // Write one byte so file is nonzero size.
    file.write("X", 1);
    return true;
}

bool data_base::initialize(const path& prefix, const chain::block& genesis)
{
    // Create paths.
    const store paths(prefix);

    if (!paths.touch_all())
        return false;

    data_base instance(paths, 0, 0);

    if (!instance.create() || !instance.start())
        return false;

    instance.push(genesis);
    return instance.stop();
}

data_base::store::store(const path& prefix)
{
    // Hash-based lookup (hash tables).
    blocks_lookup = prefix / "block_table";
    history_lookup = prefix / "history_table";
    spends_lookup = prefix / "spend_table";
    transactions_lookup = prefix / "transaction_table";

    // Height-based (reverse) lookup.
    blocks_index = prefix / "block_index";
    stealth_index = prefix / "stealth_index";

    // One (address) to many (rows).
    history_rows = prefix / "history_rows";
    stealth_rows = prefix / "stealth_rows";

    // Exclusive database access reserved by this process.
    database_lock = prefix / "process_lock";
}

bool data_base::store::touch_all() const
{
    return
        touch_file(blocks_lookup) &&
        touch_file(blocks_index) &&
        touch_file(history_lookup) &&
        touch_file(history_rows) &&
        touch_file(stealth_index) &&
        touch_file(stealth_rows) &&
        touch_file(spends_lookup) &&
        touch_file(transactions_lookup);
}

data_base::file_lock data_base::initialize_lock(const path& lock)
{
    // Touch the lock file to ensure its existence.
    const auto lock_file_path = lock.string();
    bc::ofstream file(lock_file_path, std::ios::app);
    file.close();

    // BOOST:
    // Opens a file lock. Throws interprocess_exception if the file does not
    // exist or there are no operating system resources. The file lock is
    // destroyed on its destruct and does not throw.
    return file_lock(lock_file_path.c_str());
}

void data_base::uninitialize_lock(const path& lock)
{
    boost::filesystem::remove(lock);
}

data_base::data_base(const settings& settings)
  : data_base(settings.directory, settings.history_start_height,
        settings.stealth_start_height)
{
}

data_base::data_base(const path& prefix, size_t history_height,
    size_t stealth_height)
  : data_base(store(prefix), history_height, stealth_height)
{
}

data_base::data_base(const store& paths, size_t history_height,
    size_t stealth_height)
  : lock_file_path_(paths.database_lock),
    history_height_(history_height),
    stealth_height_(stealth_height),
    sequential_lock_(0),
    mutex_(std::make_shared<shared_mutex>()),
    blocks(paths.blocks_lookup, paths.blocks_index, mutex_),
    history(paths.history_lookup, paths.history_rows, mutex_),
    stealth(paths.stealth_index, paths.stealth_rows, mutex_),
    transactions(paths.transactions_lookup, mutex_),
    spends(paths.spends_lookup, mutex_)
{
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// TODO: merge this with file creation (initialization above).
// This is actually first initialization of existing files, not file creation.
bool data_base::create()
{
    blocks.create();
    history.create();
    spends.create();
    stealth.create();
    transactions.create();
    return true;
}

bool data_base::start()
{
    // TODO: create a class to encapsulate the full file lock concept.
    file_lock_ = std::make_shared<file_lock>(initialize_lock(lock_file_path_));

    // BOOST:
    // Effects: The calling thread tries to acquire exclusive ownership of the
    // mutex without waiting. If no other thread has exclusive, or sharable
    // ownership of the mutex this succeeds. Returns: If it can acquire
    // exclusive ownership immediately returns true. If it has to wait, returns
    // false. Throws: interprocess_exception on error. Note that a file lock
    // can't guarantee synchronization between threads of the same process so
    // just use file locks to synchronize threads from different processes.
    if (!file_lock_->try_lock())
        return false;

    const auto start_exclusive = begin_write();
    blocks.start();
    history.start();
    spends.start();
    stealth.start();
    transactions.start();
    const auto end_exclusive = end_write();

    return start_exclusive && end_exclusive;
}

bool data_base::stop()
{
    const auto start_exclusive = begin_write();
    const auto result = 
        blocks.stop() &&
        history.stop() &&
        spends.stop() &&
        stealth.stop() &&
        transactions.stop();
    const auto end_exclusive = end_write();

    // This should remove the lock file. This is not important for locking
    // purposes, but it provides a sentinel to indicate hard shutdown.
    file_lock_ = nullptr;
    uninitialize_lock(lock_file_path_);

    // Return the cumulative result of the database shutdowns.
    return start_exclusive && result && end_exclusive;
}

// Locking.
// ----------------------------------------------------------------------------

handle data_base::begin_read()
{
    return sequential_lock_.load();
}

bool data_base::is_read_valid(handle value)
{
    return value == sequential_lock_.load();
}

bool data_base::is_write_locked(handle value)
{
    return (value % 2) == 1;
}

// TODO: drop a file as a write sentinel that we can use to detect uncontrolled
// shutdown during write. Use a similar approach around initial block download.
// Fail startup if the sentinel is detected. (file: write_lock).
bool data_base::begin_write()
{
    // slock is now odd.
    return is_write_locked(++sequential_lock_);
}

// TODO: clear the write sentinel.
bool data_base::end_write()
{
    // slock_ is now even again.
    return !is_write_locked(++sequential_lock_);
}

// Query engines.
// ----------------------------------------------------------------------------

static size_t get_next_height(const block_database& blocks)
{
    size_t current_height;
    const auto empty_chain = !blocks.top(current_height);
    return empty_chain ? 0 : current_height + 1;
}

static bool is_allowed_duplicate(const header& head, size_t height)
{
    return
        (height == exception1.height() && head.hash() == exception1.hash()) ||
        (height == exception2.height() && head.hash() == exception2.hash());
}

void data_base::synchronize()
{
    spends.sync();
    history.sync();
    stealth.sync();
    transactions.sync();
    blocks.sync();
}

void data_base::push(const block& block)
{
    // Height is unsafe unless database locked.
    push(block, get_next_height(blocks));
}

void data_base::push(const block& block, uint64_t height)
{
    for (size_t index = 0; index < block.transactions.size(); ++index)
    {
        // Skip BIP30 allowed duplicates (coinbase txs of excepted blocks).
        // We handle here because this is the lowest public level exposed.
        if (index == 0 && is_allowed_duplicate(block.header, height))
            continue;

        const auto& tx = block.transactions[index];
        ////const auto tx_hash = tx.hash();

        ////// Add inputs
        ////if (!tx.is_coinbase())
        ////    push_inputs(tx_hash, height, tx.inputs);

        ////// Add outputs
        ////push_outputs(tx_hash, height, tx.outputs);

        ////// Add stealth outputs
        ////push_stealth(tx_hash, height, tx.outputs);

        // Add transaction
        transactions.store(height, index, tx);
    }

    // Add block itself.
    blocks.store(block, height);

    // Synchronise everything that was added.
    synchronize();
}

void data_base::push_inputs(const hash_digest& tx_hash, size_t height,
    const input::list& inputs)
{
    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        const auto& input = inputs[index];
        const chain::input_point spend{ tx_hash, index };
        spends.store(input.previous_output, spend);

        if (height < history_height_)
            continue;

        // Try to extract an address.
        const auto address = payment_address::extract(input.script);
        if (!address)
            continue;

        history.add_spend(address.hash(), input.previous_output, spend,
            height);
    }
}

void data_base::push_outputs(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (height < history_height_)
        return;

    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto& output = outputs[index];
        const chain::output_point outpoint{ tx_hash, index };

        // Try to extract an address.
        const auto address = payment_address::extract(output.script);
        if (!address)
            continue;

        history.add_output(address.hash(), outpoint, height,
            output.value);
    }
}

void data_base::push_stealth(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (height < stealth_height_)
        return;

    BITCOIN_ASSERT_MSG(outputs.size() <= max_int64, "overflow");
    const auto outputs_size = static_cast<int64_t>(outputs.size());

    // Stealth cannot be in last output because it is paired.
    for (int64_t index = 0; index < (outputs_size - 1); ++index)
    {
        const auto& ephemeral_script = outputs[index].script;
        const auto& payment_script = outputs[index + 1].script;

        // Try to extract an unsigned ephemeral key from the odd output.
        hash_digest unsigned_ephemeral_key;
        if (!extract_ephemeral_key(unsigned_ephemeral_key, ephemeral_script))
            continue;

        // Try to extract a stealth prefix from the odd output.
        uint32_t prefix;
        if (!to_stealth_prefix(prefix, ephemeral_script))
            continue;

        // Try to extract the payment address from the even output.
        // The payment address versions are arbitrary and unused here.
        const auto address = payment_address::extract(payment_script);
        if (!address)
            continue;

        const chain::stealth_row row
        {
            unsigned_ephemeral_key,
            address.hash(),
            tx_hash
        };

        stealth.store(prefix, row);
    }
}

chain::block data_base::pop()
{
    size_t height;
    if (!blocks.top(height))
        throw std::runtime_error("The chain is empty.");

    const auto block_result = blocks.get(height);

    // Set result header.
    chain::block block;
    block.header = block_result.header();
    const auto count = block_result.transaction_count();

    // TODO: unreverse the loop so we can avoid this.
    BITCOIN_ASSERT_MSG(count <= max_int64, "overflow");
    const auto unsigned_count = static_cast<int64_t>(count);

    // Loop backwards (in reverse to how we added).
    for (int64_t index = unsigned_count - 1; index >= 0; --index)
    {
        const auto tx_hash = block_result.transaction_hash(index);
        const auto tx_result = transactions.get(tx_hash);
        BITCOIN_ASSERT(tx_result);
        BITCOIN_ASSERT(tx_result.height() == height);
        BITCOIN_ASSERT(tx_result.index() == static_cast<size_t>(index));

        const auto tx = tx_result.transaction();

        // Do things in reverse so pop txs, then outputs, then inputs.
        transactions.remove(tx_hash);

        // Remove outputs
        pop_outputs(tx.outputs, height);

        // Remove inputs
        if (!tx.is_coinbase())
            pop_inputs(tx.inputs, height);

        // Add transaction to result
        block.transactions.push_back(tx);
    }

    stealth.unlink(height);
    blocks.unlink(height);

    // Synchronise everything that was changed.
    synchronize();

    // Reverse, since we looped backwards.
    std::reverse(block.transactions.begin(), block.transactions.end());
    return block;
}

void data_base::pop_inputs(const input::list& inputs, size_t height)
{
    BITCOIN_ASSERT_MSG(inputs.size() <= max_int64, "overflow");
    const auto inputs_size = static_cast<int64_t>(inputs.size());

    // Loop in reverse.
    for (int64_t index = inputs_size - 1; index >= 0; --index)
    {
        const auto& input = inputs[index];
        spends.remove(input.previous_output);

        if (height < history_height_)
            continue;

        // Try to extract an address.
        const auto address = payment_address::extract(input.script);
        if (address)
            history.delete_last_row(address.hash());
    }
}

void data_base::pop_outputs(const output::list& outputs, size_t height)
{
    if (height < history_height_)
        return;

    BITCOIN_ASSERT_MSG(outputs.size() <= max_int64, "overflow");
    const auto outputs_size = static_cast<int64_t>(outputs.size());

    // Loop in reverse.
    for (int64_t index = outputs_size - 1; index >= 0; --index)
    {
        const auto& output = outputs[index];

        // Try to extract an address.
        const auto address = payment_address::extract(output.script);
        if (address)
            history.delete_last_row(address.hash());
    }
}

} // namespace data_base
} // namespace libbitcoin
