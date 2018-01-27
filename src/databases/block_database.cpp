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
#include <bitcoin/database/databases/block_database.hpp>

#include <cstdint>
#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/result/block_result.hpp>
#include <bitcoin/database/state/block_state.hpp>

// Record format (v4) [99 bytes, 135 with key/link]:
// Below excludes block height and tx hash indexes (arrays).
// ----------------------------------------------------------------------------
// [ header:80          - const   ]
// [ median_time_past:4 - const   ]
// [ height:4           - const   ] (in any branch)
// [ state:1            - atomic1 ] (invalid, empty, stored, pooled, indexed, confirmed)
// [ checksum/code:4    - atomic2 ] (optional, zero if not cached, code if invalid)
// [ tx_start:4         - atomic3 ] (array index into the transaction_index, or zero)
// [ tx_count:2         - atomic3 ] (atomic with start, zero if block unpopulated)

// Record format (v3) [variable bytes] (median_time_past added in v3.3):
// Below excludes block height index (array).
// ----------------------------------------------------------------------------
// [ header:80          - const ]
// [ median_time_past:4 - const ]
// [ height:4           - const ]
// [ tx_count:1-2       - const ]
// [ [ tx_hash:32 ]...  - const ]

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static const auto header_size = header::satoshi_fixed_size();
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto state_size = sizeof(uint8_t);
static constexpr auto checksum_size = sizeof(uint32_t);
static constexpr auto tx_start_size = sizeof(uint32_t);
static constexpr auto tx_count_size = sizeof(uint16_t);

static const auto height_offset = header_size + median_time_past_size;
static const auto state_offset = height_offset + height_size;
static const auto checksum_offset = state_offset + state_size;
static const auto transactions_offset = checksum_offset + checksum_size;

// Placeholder for unimplemented checksum caching.
static constexpr auto no_checksum = 0u;

// Total size of block header and metadta storage.
static const auto block_size = header_size + median_time_past_size +
    height_size + state_size + checksum_size + tx_start_size + tx_count_size;

// Blocks uses a hash table and two array indexes, all O(1).
// The block database keys off of block hash and has block value.
block_database::block_database(const path& map_filename,
    const path& header_index_filename, const path& block_index_filename,
    const path& tx_index_filename, size_t buckets, size_t expansion)
  : fork_point_(0),
    valid_point_(0),

    hash_table_file_(map_filename, expansion),
    hash_table_(hash_table_file_, buckets, block_size),

    // Array storage.
    header_index_file_(header_index_filename, expansion),
    header_index_(header_index_file_, 0, sizeof(link_type)),

    // Array storage.
    block_index_file_(block_index_filename, expansion),
    block_index_(block_index_file_, 0, sizeof(link_type)),

    // Array storage.
    tx_index_file_(tx_index_filename, expansion),
    tx_index_(tx_index_file_, 0, sizeof(file_offset))
{
}

block_database::~block_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool block_database::create()
{
    if (!hash_table_file_.open() ||
        !header_index_file_.open() ||
        !block_index_file_.open() ||
        !tx_index_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create() &&
        header_index_.create() &&
        block_index_.create() &&
        tx_index_.create();
}

bool block_database::open()
{
    return
        hash_table_file_.open() &&
        header_index_file_.open() &&
        block_index_file_.open() &&
        tx_index_file_.open() &&

        hash_table_.start() &&
        header_index_.start() &&
        block_index_.start() &&
        tx_index_.start();
}

void block_database::commit()
{
    hash_table_.commit();
    header_index_.commit();
    block_index_.commit();
    tx_index_.commit();
}

bool block_database::flush() const
{
    return
        hash_table_file_.flush() &&
        header_index_file_.flush() &&
        block_index_file_.flush() &&
        tx_index_file_.flush();
}

bool block_database::close()
{
    return
        hash_table_file_.close() &&
        header_index_file_.close() &&
        block_index_file_.close() &&
        tx_index_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

size_t block_database::fork_point() const
{
    return fork_point_;
}

size_t block_database::valid_point() const
{
    return valid_point_;
}

bool block_database::top(size_t& out_height, bool block_index) const
{
    auto& manager = block_index ? block_index_ : header_index_;
    return read_top(out_height, manager);
}

block_result block_database::get(size_t height, bool block_index) const
{
    auto& manager = block_index ? block_index_ : header_index_;

    return
    {
        // This is not guarded for an invalid offset.
        hash_table_.find(read_index(height, manager)),
        metadata_mutex_,
        tx_index_
    };
}

// Returns any state, including invalid and empty.
block_result block_database::get(const hash_digest& hash) const
{
    return
    {
        hash_table_.find(hash),
        metadata_mutex_,
        tx_index_
    };
}

// Store.
// ----------------------------------------------------------------------------

// private
void block_database::push(const chain::header& header, size_t height,
    uint32_t checksum, link_type tx_start, size_t tx_count, uint8_t state)
{
    auto& manager = is_confirmed(state) ? block_index_ : header_index_;

    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);
    BITCOIN_ASSERT(!header.validation.pooled);

    const auto writer = [&](byte_serializer& serial)
    {
        header.to_data(serial, false);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_byte(state);
        serial.write_4_bytes_little_endian(checksum);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
    };

    // Write the new block.
    auto front = hash_table_.allocator();
    const auto link = front.create(header.hash(), writer);
    hash_table_.link(front);

    if (is_confirmed(state) || is_indexed(state))
        push_index(link, height, manager);
}

// A header creation does not move the fork point (not a reorg).
void block_database::push(const chain::header& header, size_t height)
{
    // Initially store header as indexed, pent download (the top header).
    static const auto state = block_state::indexed | block_state::pent;

    // The header/block already exists, promote from pooled to indexed.
    if (header.validation.pooled)
    {
        confirm(header.hash(), height, false);
        return;
    }

    push(header, height, no_checksum, 0, 0, state);
}

// This creates a new store entry even if a previous existed.
// A block creation does not move the fork point (not a reorg).
void block_database::push(const chain::block& block, size_t height)
{
    // Initially store block as confirmed-valid (the top block).
    static const auto state = block_state::confirmed | block_state::valid;

    const auto& header = block.header();
    const auto& txs = block.transactions();
    push(header, height, no_checksum, associate(txs), txs.size(), state);
}

block_database::link_type block_database::associate(
    const transaction::list& transactions)
{
    if (transactions.empty())
        return 0;

    const auto start = tx_index_.allocate(transactions.size());
    const auto record = tx_index_.get(start);
    auto serial = make_unsafe_serializer(record->buffer());

    for (const auto& tx: transactions)
        serial.write_8_bytes_little_endian(tx.validation.link);

    return start;
}

// Update.
// ----------------------------------------------------------------------------
// These are used to atomically update values 

// Populate transaction references, state is unchanged.
bool block_database::update(const chain::block& block)
{
    const auto& txs = block.transactions();
    const auto tx_start = associate(txs);
    const auto tx_count = txs.size();

    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);

    const auto updater = [&](byte_serializer& serial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.skip(transactions_offset);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
        ///////////////////////////////////////////////////////////////////////
    };

    auto element = hash_table_.find(block.hash());

    if (!element)
        return false;

    element.write(updater);
    return true;
}

static uint8_t update_validation_state(uint8_t original, bool positive)
{
    // May only validate or invalidate a pooled or indexed block.
    BITCOIN_ASSERT(is_pooled(original) || is_indexed(original));

    // Preserve the confirmation state (pooled or indexed).
    // We try to only validate indexed blocks, but allow pooled for a race.
    const auto confirmation_state = original & block_state::confirmations;
    const auto validation_state = positive ? block_state::valid :
        block_state::failed;

    // Merge the new confirmation state with existing validation state.
    return confirmation_state | validation_state;
}

// Promote pent block to valid|invalid.
// TODO: the caller doesn't know the current header state.
bool block_database::validate(const hash_digest& hash, bool positive)
{
    auto element = hash_table_.find(hash);

    if (!element)
        return false;

    uint32_t height;
    uint8_t state;

    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(height_offset);
        height = deserial.read_4_bytes_little_endian();

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        state = deserial.read_byte();
        ///////////////////////////////////////////////////////////////////////
    };

    const auto updater = [&](byte_serializer& serial)
    {
        serial.skip(state_offset);
        const auto updated = update_validation_state(state, positive);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_byte(updated);
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    element.write(updater);

    // Also update the validation chaser, assumes all prior are valid.
    BITCOIN_ASSERT_MSG(valid_point_ != max_size_t, "valid point overflow");
    BITCOIN_ASSERT_MSG(
        (height == 0 && valid_point_ == 0) || (height == valid_point_ + 1),
        "validation out of order");

    valid_point_ = height;
    return true;
}

static uint8_t update_confirmation_state(uint8_t original, bool positive,
    bool block_index)
{
    // May only confirm a valid block.
    BITCOIN_ASSERT(!positive || !block_index || is_valid(original));

    // May only unconfirm via block indexing.
    BITCOIN_ASSERT(positive || !block_index || !is_confirmed(original));

    // May only index a pooled header.
    BITCOIN_ASSERT(!positive || block_index || is_pooled(original));

    // May only deindex via header indexing.
    BITCOIN_ASSERT(positive || block_index || !is_indexed(original));

    // Preserve the validation state (header-indexed blocks can be pent).
    const auto validation_state = original & block_state::validations;
    const auto positive_state = block_index ? block_state::confirmed :
        block_state::indexed;

    // Demotion is always directly to the pooled state.
    const auto confirmation_state = positive ? positive_state :
        block_state::pooled;

    // Merge the new confirmation state with existing validation state.
    return confirmation_state | validation_state;
}

// TODO: the caller doesn't know the current header state.
bool block_database::confirm(const hash_digest& hash, size_t height,
    bool block_index)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = block_index ? block_index_ : header_index_;

    // Can only confirm at the top of the given index (push).
    if (height != manager.count())
        return false;

    auto element = hash_table_.find(read_index(height, manager));

    if (!element)
        return false;

    uint8_t original;

    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(state_offset);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        original = deserial.read_byte();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    auto updated = update_confirmation_state(original, true, block_index);

    const auto updater = [&](byte_serializer& serial)
    {
        serial.skip(state_offset);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_byte(updated);
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(updater);

    // Increment fork point for block-indexed confirmation.
    if (block_index && is_confirmed(updated))
    {
        BITCOIN_ASSERT_MSG(fork_point_ != max_size_t, "fork point overflow");
        ++fork_point_;
    }

    push_index(element.link(), height, manager);
    return true;
}

// TODO: the caller already knows the current header state.
bool block_database::unconfirm(const hash_digest& hash, size_t height,
    bool block_index)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = block_index ? block_index_ : header_index_;

    // Can only unconfirm the top of the given index (pop).
    if (height + 1u != manager.count())
        return false;

    auto element = hash_table_.find(read_index(height, manager));

    if (!element)
        return false;

    uint8_t original;

    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(state_offset);

        ///////////////////////////////////////////////////////////////////////
        // Critical Section.
        shared_lock lock(metadata_mutex_);
        original = deserial.read_byte();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    auto updated = update_confirmation_state(original, false, block_index);

    const auto updater = [&](byte_serializer& serial)
    {
        serial.skip(state_offset);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_byte(updated);
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(updater);

    // Decrement fork point for previously-confirmed block via header-index.
    if (!block_index && is_confirmed(original))
    {
        BITCOIN_ASSERT_MSG(fork_point_ != 0, "fork point underflow");
        --fork_point_;
    }

    pop_index(height, manager);
    return true;
}

// Index Utilities.
// ----------------------------------------------------------------------------

bool block_database::read_top(size_t& out_height,
    const record_manager& manager) const
{
    const auto count = manager.count();

    // Guard against no genesis block.
    if (count == 0)
        return false;

    out_height = count - 1;
    return true;
}

block_database::link_type block_database::read_index(size_t height,
    const record_manager& manager) const
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height < manager.count());

    const auto height32 = static_cast<uint32_t>(height);
    const auto record = manager.get(height32);
    return from_little_endian_unsafe<link_type>(record->buffer());
}

void block_database::pop_index(size_t height, record_manager& manager)
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height + 1u == manager.count());

    const auto height32 = static_cast<uint32_t>(height);
    manager.set_count(height32);
}

void block_database::push_index(link_type index, size_t height,
    record_manager& manager)
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height == manager.count());

    manager.allocate(1);
    const auto height32 = static_cast<uint32_t>(height);
    const auto record = manager.get(height32);
    auto serial = make_unsafe_serializer(record->buffer());
    serial.write_4_bytes_little_endian(index);
}

} // namespace database
} // namespace libbitcoin
