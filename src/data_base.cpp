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
#include <bitcoin/database/data_base.hpp>

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <boost/filesystem.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/result/block_result.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>
#include <bitcoin/database/verify.hpp>

namespace libbitcoin {
namespace database {

using namespace std::placeholders;
using namespace boost::filesystem;
using namespace bc::system;
using namespace bc::system::chain;
using namespace bc::system::machine;
using namespace bc::system::wallet;

#define NAME "data_base"

// TODO: replace spends with complex query, output gets inpoint:
// (1) transactions_.get(outpoint, require_confirmed)->spender_height.
// (2) blocks_.get(spender_height)->transactions().
// (3) (transactions()->inputs()->previous_output() == outpoint)->inpoint.
// This has the same average cost as 1 output-query + 1/2 block-query.
// This will reduce server indexing by 30% (payment indexing only).
// Could make index optional, redirecting queries if not present.

// A failure after begin_write is returned without calling end_write.
// This leaves the local flush lock enabled, preventing usage after restart.

// Construct.
// ----------------------------------------------------------------------------

data_base::data_base(const settings& settings, bool catalog, bool filter)
  : closed_(true),
    catalog_(catalog),
    filter_(filter),
    settings_(settings),
    database::store(settings.directory, catalog, filter_, settings.flush_writes)
{
    LOG_DEBUG(LOG_DATABASE)
        << "Buckets: "
        << "block [" << settings.block_table_buckets << "], "
        << "transaction [" << settings.transaction_table_buckets << "], "
        << "payment [" << settings.payment_table_buckets << "]";
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
    if (!create())
        return false;

    start();

    // These leave the databases open.
    auto created = blocks_->create() && transactions_->create();

    if (filter_)
        created &= filters_->create();

    if (catalog_)
        created &= payments_->create();

    created &= push(genesis) == error::success;

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

    if (filter_)
        opened &= filters_->open() && populate_filter_cache(*filters_);

    if (catalog_)
        opened &= payments_->open();

    if (!opened)
        return false;

    closed_ = false;
    return opened;
}

// TODO: simplify interface by passing settings reference to databases.

// protected
void data_base::start()
{
    blocks_ = std::make_shared<block_database>(
        block_table,
        candidate_index,
        confirmed_index,
        transaction_index,
        settings_.block_table_size,
        settings_.candidate_index_size,
        settings_.confirmed_index_size,
        settings_.transaction_index_size,
        settings_.block_table_buckets,
        settings_.file_growth_rate,
        filter_);

    transactions_ = std::make_shared<transaction_database>(
        transaction_table,
        settings_.transaction_table_size,
        settings_.transaction_table_buckets,
        settings_.file_growth_rate,
        settings_.cache_capacity);

    if (filter_)
    {
        filters_ = std::make_shared<filter_database>(
            neutrino_filter_table,
            settings_.neutrino_filter_table_size,
            settings_.neutrino_filter_table_buckets,
            settings_.file_growth_rate,
            neutrino_filter_type);
    }

    if (catalog_)
    {
        payments_ = std::make_shared<payment_database>(
            payment_table,
            payment_rows,
            settings_.payment_table_size,
            settings_.payment_index_size,
            settings_.payment_table_buckets,
            settings_.file_growth_rate);
    }
}

// protected
void data_base::commit()
{
    if (catalog_)
        payments_->commit();

    if (filter_)
        filters_->commit();

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

    if (filter_)
        flushed &= filters_->flush();

    if (catalog_)
        flushed &= payments_->flush();

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

    if (filter_)
        closed &= filters_->close();

    if (catalog_)
        closed &= payments_->close();

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

// Invalid if neutrino filters not initialized.
const filter_database& data_base::neutrino_filters() const
{
    return *filters_;
}

// Invalid if indexes not initialized.
const payment_database& data_base::payments() const
{
    return *payments_;
}

// Public writers.
// ----------------------------------------------------------------------------

code data_base::catalog(const transaction& tx)
{
    code ec;

    // Existence check prevents duplicated indexing.
    if (!catalog_ || tx.metadata.cataloged)
        return ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    if ((ec = verify_exists(*transactions_, tx)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    payments_->catalog(tx);
    payments_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Called from candidate and push.
code data_base::catalog(const block& block)
{
    code ec;
    if (!catalog_)
        return ec;

    const auto start = asio::steady_clock::now();

    // Existence checks prevent duplicated indexing.
    for (const auto& tx: block.transactions())
        if (!tx.metadata.cataloged)
            payments_->catalog(tx);

    payments_->commit();

    block.metadata.catalog = asio::steady_clock::now() - start;
    return error::success;
}

// Called from candidate and push.
system::code data_base::filter(const block& block)
{
    code ec;
    if (!filter_)
        return ec;

    const auto start = asio::steady_clock::now();

    const auto neutrino_filter = block.header().metadata.neutrino_filter;

    if (!neutrino_filter)
        return error::operation_failed;

    const auto link = neutrino_filter->metadata.link;
    const auto exists = link != block_filter::validation::unlinked;

    // Existence check prevents duplicated indexing.
    if (!exists)
    {
        if (!filters_->store(*neutrino_filter))
            return error::operation_failed;

        if (!blocks_->update_neutrino_filter(block.hash(), link))
            return error::operation_failed;
    }

    filters_->commit();

    block.metadata.filter = asio::steady_clock::now() - start;
    return error::success;
}

code data_base::store(const transaction& tx, uint32_t forks)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    // Returns error::duplicate_transaction if tx with same hash exists.
    if ((ec = verify_missing(*transactions_, tx)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Store the transaction if missing and always set tx link metadata.
    if (!transactions_->store(tx, forks))
        return error::operation_failed;

    // TODO: add the tx to unspent transaction cache as unconfirmed.

    transactions_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

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

code data_base::confirm(const hash_digest& block_hash, size_t height)
{
    code ec;

    if ((ec = verify_confirm(*blocks_, block_hash, height)))
        return error::operation_failed;

    const auto block = blocks().get(block_hash);
    const auto time = block.median_time_past();
    size_t position = 0;

    // Mark block txs as confirmed without reading transactions.
    for (const auto tx_offset: block)
        if (!transactions_->confirm(tx_offset, height, time, position++))
            return error::operation_failed;

    // TODO: optimize using link.
    // Promote block to confirmed.
    if (!blocks_->promote(block_hash, height, false))
        return error::operation_failed;

    return error::success;
}

// Add missing transactions for an existing block header.
// This allows parallel write when write flushing is not enabled.
code data_base::update(const chain::block& block, size_t height)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    const auto start = asio::steady_clock::now();
    if ((ec = verify_update(*blocks_, block, height)))
        return ec;

    // TODO: This could be skipped when stored header's tx count is non-zero.

    // Conditional write mutex preserves write flushing by preventing overlap.
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Store the missing transactions and set tx link metadata for all.
    if (!transactions_->store(block.transactions()))
        return error::operation_failed;

    // Store the block's filter data (header, filter).

    // Update the block's transaction associations (not its state).
    if (!blocks_->update_transactions(block))
        return error::operation_failed;

    transactions_->commit();

    block.metadata.associate = asio::steady_clock::now() - start;
    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Promote unvalidated block to valid|invalid based on error value.
code data_base::invalidate(const header& header, const code& error)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    if ((ec = verify_exists(*blocks_, header)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    if (!blocks_->validate(header.hash(), error))
        return error::operation_failed;

    header.metadata.error = error;
    header.metadata.validated = true;

    return end_write() ? error::success : error::store_lock_failure;
    ///////////////////////////////////////////////////////////////////////////
}

// Mark candidate as valid, and txs and outputs spent by them as candidate.
code data_base::candidate(const block& block)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(flush_each_write());

    const auto start = asio::steady_clock::now();
    if ((ec = verify_not_failed(*blocks_, block)))
        return ec;

    const auto& header = block.header();
    BITCOIN_ASSERT(!header.metadata.error);

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Set candidate validation state to valid.
    if (!blocks_->validate(header.hash(), error::success))
        return error::operation_failed;

    // Mark candidate block txs and outputs spent by them as candidate.
    for (const auto& tx: block.transactions())
        if (!transactions_->candidate(tx.metadata.link))
            return error::operation_failed;

    if ((ec = filter(block)))
        return ec;

    if ((ec = catalog(block)))
        return ec;

    header.metadata.error = error::success;
    header.metadata.validated = true;

    block.metadata.candidate = asio::steady_clock::now() - start;
    return end_write() ? error::success : error::store_lock_failure;
    ///////////////////////////////////////////////////////////////////////////
}

// Reorganize blocks.
// Header metadata median_time_past must be set on all incoming blocks.
code data_base::reorganize(const config::checkpoint& fork_point,
    block_const_ptr_list_const_ptr incoming,
    block_const_ptr_list_ptr outgoing)
{
    if (fork_point.height() > max_size_t - incoming->size())
        return error::operation_failed;

    const auto result =
        pop_above(outgoing, fork_point) &&
        push_all(incoming, fork_point);

    if (!result)
        return error::operation_failed;

    if (filter_)
        return update_filter_cache(*filters_, fork_point, incoming, outgoing);

    return error::success;
}

// Store, update, validate and confirm the presumed valid block.
code data_base::push(const block& block, size_t height,
    uint32_t median_time_past)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Store the header.
    blocks_->store(block.header(), height, median_time_past);

    // Push header reference onto the candidate index and set candidate state.
    if (!blocks_->promote(block.hash(), height, true))
        return error::operation_failed;

    // Store any missing txs as unconfirmed, set tx link metadata for all.
    if (!transactions_->store(block.transactions()))
        return error::operation_failed;

    // Populate transaction references from link metadata.
    if (!blocks_->update_transactions(block))
        return error::operation_failed;

    // Confirm all transactions (candidate state transition not requried).
    if (!transactions_->confirm(block, height, median_time_past))
        return error::operation_failed;

    // Promote validation state to valid (presumed valid).
    if (!blocks_->validate(block.hash(), error::success))
        return error::operation_failed;

    if ((ec = filter(block)))
        return ec;

    if ((ec = catalog(block)))
        return ec;

    // TODO: optimize using link.
    // Push header reference onto the confirmed index and set confirmed state.
    if (!blocks_->promote(block.hash(), height, false))
        return error::operation_failed;

    blocks_->commit();
    transactions_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Header reorganization.
// ----------------------------------------------------------------------------
// protected

bool data_base::push_all(header_const_ptr_list_const_ptr headers,
    const config::checkpoint& fork_point)
{
    code ec;
    const auto first_height = fork_point.height() + 1;

    // Push all headers onto the fork point.
    for (size_t index = 0; index < headers->size(); ++index)
    {
        const auto& header = *((*headers)[index]);
        const auto median_time_past = header.metadata.median_time_past;
        if ((ec = push_header(header, first_height + index, median_time_past)))
            return false;
    }

    return true;
}

bool data_base::pop_above(header_const_ptr_list_ptr headers,
    const config::checkpoint& fork_point)
{
    code ec;
    headers->clear();
    if ((ec = verify(*blocks_, fork_point, true)))
        return false;

    size_t top;
    if (!blocks_->top(top, true))
        return false;

    const auto fork = fork_point.height();
    const auto depth = top - fork;
    headers->reserve(depth);
    if (depth == 0)
        return true;

    // Pop all headers above the fork point.
    for (size_t height = top; height > fork; --height)
    {
        const auto next = std::make_shared<messages::header>();
        if ((ec = pop_header(*next, height)))
            return false;

        headers->insert(headers->begin(), next);
    }

    return true;
}

// Expects header is next candidate and metadata.exists is populated.
// Median time past metadata is populated when the block is validated.
code data_base::push_header(const chain::header& header, size_t height,
    uint32_t median_time_past)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_push(*blocks_, header, height)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    if (!header.metadata.exists)
        blocks_->store(header, height, median_time_past);

    // TODO: optimize using link.
    blocks_->promote(header.hash(), height, true);
    blocks_->commit();

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Expects header exists at the top of the candidate index.
code data_base::pop_header(chain::header& out_header, size_t height)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_top(*blocks_, height, true)))
        return ec;

    const auto result = blocks_->get(height, true);

    if (!result)
        return error::operation_failed;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Uncandidate previous outputs spent by txs of this candidate block.
    for (const auto link: result)
        if (!transactions_->uncandidate(link))
            return error::operation_failed;

    // TODO: optimize using link.
    // Demote the candidate header.
    if (!blocks_->demote(result.hash(), height, true))
        return error::operation_failed;

    blocks_->commit();

    out_header = result.header();
    BITCOIN_ASSERT(out_header.is_valid());

    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

// Block reorganization.
// ----------------------------------------------------------------------------
// protected

bool data_base::push_all(block_const_ptr_list_const_ptr blocks,
    const config::checkpoint& fork_point)
{
    code ec;
    const auto first_height = fork_point.height() + 1;

    // Push all blocks onto the fork point.
    for (size_t index = 0; index < blocks->size(); ++index)
    {
        const auto& block = *((*blocks)[index]);
        if ((ec = push_block(block, first_height + index)))
            return false;
    }

    return true;
}

bool data_base::pop_above(block_const_ptr_list_ptr blocks,
    const config::checkpoint& fork_point)
{
    code ec;
    blocks->clear();
    if ((ec = verify(*blocks_, fork_point, false)))
        return false;

    size_t top;
    if (!blocks_->top(top, false))
        return false;

    const auto fork = fork_point.height();
    const auto depth = top - fork;
    blocks->reserve(depth);
    if (depth == 0)
        return true;

    // Pop all blocks above the fork point.
    for (size_t height = top; height > fork; --height)
    {
        const auto next = std::make_shared<messages::block>();
        if ((ec = pop_block(*next, height)))
            return false;

        blocks->insert(blocks->begin(), next);
    }

    return true;
}

code data_base::push_block(const block& block, size_t height)
{
    code ec;
    const auto median_time_past = block.header().metadata.median_time_past;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    const auto start = asio::steady_clock::now();
    if ((ec = verify_push(*blocks_, block, height)))
        return ec;

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Confirm txs (and thereby also payment indexes), spend prevouts.
    if (!transactions_->confirm(block, height, median_time_past))
        return error::operation_failed;

    // TODO: optimize using link.
    // Confirm candidate block (candidate index unchanged).
    if (!blocks_->promote(block.hash(), height, false))
        return error::operation_failed;

    blocks_->commit();

    block.metadata.confirm = asio::steady_clock::now() - start;
    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

code data_base::pop_block(chain::block& out_block, size_t height)
{
    code ec;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    if ((ec = verify_top(*blocks_, height, false)))
        return ec;

    const auto result = blocks_->get(height, false);

    if (!result)
        return error::operation_failed;

    // Create a block for walking transactions and return.
    out_block = chain::block(result.header(), to_transactions(result));
    BITCOIN_ASSERT(out_block.hash() == result.hash());

    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if (!begin_write())
        return error::store_lock_failure;

    // Deconfirm txs (and thereby also payment indexes), unspend prevouts.
    if (!transactions_->unconfirm(out_block))
        return error::operation_failed;

    // TODO: optimize using link.
    // Demote the confirmed block (candidate index unchanged).
    if (!blocks_->demote(result.hash(), height, false))
        return error::operation_failed;

    blocks_->commit();

    BITCOIN_ASSERT(out_block.is_valid());
    return end_write() ? error::success : error::store_lock_failure;
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////
}

system::code data_base::populate_filter_cache(filter_database& database)
{
    constexpr auto interval = compact_filter_checkpoint_interval;
    size_t height = 0u;

    if (!blocks().top(height, false))
        return error::operation_failed;

    hash_list checkpoints;
    checkpoints.reserve(height / interval);

    for (auto index = interval; index <= height;
        index = ceiling_add(index, interval))
    {
        const auto block_result = blocks().get(index, false);

        if (!block_result)
            return error::operation_failed;

        const auto filter_result = database.get(
            block_result.neutrino_filter());

        if (!filter_result)
            return error::operation_failed;

        checkpoints.push_back(filter_result.header());
    }

    database.set_checkpoints(std::move(checkpoints));
    return error::success;
}

// TODO: incorporate into reorg loops using safe cache object, adding a call
// each to push_block/pop_block (or promote/demote).
system::code data_base::update_filter_cache(filter_database& database,
    const system::config::checkpoint& fork_point,
    system::block_const_ptr_list_const_ptr incoming,
    system::block_const_ptr_list_ptr outgoing)
{
    constexpr auto interval = compact_filter_checkpoint_interval;
    auto checkpoints = database.checkpoints();
    auto changed = false;

    if (!outgoing->empty())
    {
        const auto previous_height = ceiling_add(fork_point.height(),
            outgoing->size());
        const auto previous_count = previous_height / interval;
        const auto fork_height_count = fork_point.height() / interval;

        if (previous_count > fork_height_count)
        {
            changed = true;
            checkpoints.resize(floor_subtract(checkpoints.size(),
                previous_count - fork_height_count));
        }
    }

    if (!incoming->empty())
    {
        auto height = ceiling_add(fork_point.height(), incoming->size());
        auto checkpoint_count = height / interval;

        if (checkpoints.size() < checkpoint_count)
        {
            changed = true;
            const auto last_height = checkpoints.size() * interval;

            for (auto index = last_height; index <= height;
                index = ceiling_add(index, interval))
            {
                const auto block_result = blocks().get(index, false);

                if (!block_result)
                    return error::operation_failed;

                const auto filter_result = database.get(
                    block_result.neutrino_filter());

                if (!filter_result)
                    return error::operation_failed;

                checkpoints.push_back(filter_result.header());
            }
        }
    }

    if (changed)
        database.set_checkpoints(std::move(checkpoints));

    return error::success;
}

// Utilities.
// ----------------------------------------------------------------------------
// protected

// Private (assumes valid result links).
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
