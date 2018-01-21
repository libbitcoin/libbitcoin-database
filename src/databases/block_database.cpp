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
#include <bitcoin/database/primitives/slab_row.hpp>
#include <bitcoin/database/result/block_result.hpp>
#include <bitcoin/database/state/block_state.hpp>

// Record format (v4) [99 bytes]:
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

// static
const size_t block_database::prefix_size_ = slab_row<key_type,
    link_type, record_manager>::prefix_size;

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
static const auto block_size = header_size + median_time_past_size +
    height_size + state_size + checksum_size + tx_start_size + tx_count_size;

// Placeholder for unimplemented checksum caching.
static constexpr auto no_checksum = 0u;

// Blocks uses a hash table and two array indexes, all O(1).
// The block database keys off of block hash and has block value.
block_database::block_database(const path& map_filename,
    const path& header_index_filename, const path& block_index_filename,
    const path& tx_index_filename, size_t buckets, size_t expansion)
  : fork_point_(0),
    valid_point_(0),

    hash_table_file_(map_filename, expansion),
    hash_table_(hash_table_file_, buckets, block_size),

    header_index_file_(header_index_filename, expansion),
    header_index_(header_index_file_, 0, sizeof(link_type)),

    block_index_file_(block_index_filename, expansion),
    block_index_(block_index_file_, 0, sizeof(link_type)),

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
    hash_table_.sync();
    header_index_.sync();
    block_index_.sync();
    tx_index_.sync();
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

    if (height >= manager.count())
        return{ tx_index_ };

    auto record = hash_table_.get(read_index(height, manager));
    const auto prefix = record->buffer();

    // Advance the record row entry past the key and link to the record data.
    record->increment(prefix_size_);
    auto deserial = make_unsafe_deserializer(record->buffer());

    // The header and height are const.
    deserial.skip(state_offset);

    // Each of the three atomic sets could be guarded independently.
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    metadata_mutex_.lock_shared();
    const auto state = deserial.read_byte();
    const auto checksum = deserial.read_4_bytes_little_endian();
    const auto tx_start = deserial.read_4_bytes_little_endian();
    const auto tx_count = deserial.read_2_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // HACK: back up into the record to obtain the hash/key (optimization).
    auto reader = make_unsafe_deserializer(prefix);

    // Reads are not deferred for updatable values as atomicity is required.
    return{ tx_index_, record, reader.read_hash(),
        static_cast<uint32_t>(height), checksum, tx_start, tx_count, state };
}

// Returns any state, including invalid and empty.
block_result block_database::get(const hash_digest& hash) const
{
    // This is pointer to the data section of the record row entry.
    const auto record = hash_table_.find(hash);

    if (record == nullptr)
        return{ tx_index_ };

    const auto memory = record->buffer();
    ////const auto prefix_start = memory - prefix_size_;

    // The header and height never change after the block is reachable.
    auto deserial = make_unsafe_deserializer(memory + height_offset);
    const auto height = deserial.read_4_bytes_little_endian();

    // Each of the three atomic sets could be guarded independently.
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    metadata_mutex_.lock_shared();
    const auto state = deserial.read_byte();
    const auto checksum = deserial.read_4_bytes_little_endian();
    const auto tx_start = deserial.read_4_bytes_little_endian();
    const auto tx_count = deserial.read_2_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Reads are not deferred for updatable values as atomicity is required.
    return{ tx_index_, record, hash, height, checksum, tx_start,
        tx_count, state };
}

// Store.
// ----------------------------------------------------------------------------

// private
void block_database::push(const chain::header& header, size_t height,
    uint32_t checksum, link_type tx_start, size_t tx_count, uint8_t state)
{
    auto& manager = is_confirmed(state) ? block_index_ :
        header_index_;

    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);
    BITCOIN_ASSERT(!header.validation.pooled);
    const auto height32 = static_cast<uint32_t>(height);

    const auto write = [&](byte_serializer& serial)
    {
        // The record is not accessible until stored, so no guards required.
        // Write block header including median_time_past metadata.
        header.to_data(serial, false);
        serial.write_4_bytes_little_endian(height32);
        serial.write_byte(state);
        serial.write_4_bytes_little_endian(checksum);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
    };

    const auto index = hash_table_.store(header.hash(), write);

    if (is_confirmed(state) || is_indexed(state))
        push_index(index, height, manager);
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
    {
        const auto offset = tx.validation.offset;
        BITCOIN_ASSERT(offset != transaction_database::slab_map::not_found);
        serial.write_8_bytes_little_endian(offset);
    }

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

    const auto update = [&](byte_serializer& serial)
    {
        ///////////////////////////////////////////////////////////////////////
        // Critical Section.
        unique_lock lock(metadata_mutex_);

        serial.skip(transactions_offset);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
        ///////////////////////////////////////////////////////////////////////
    };

    return hash_table_.update(block.hash(), update) != record_map::not_found;
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
bool block_database::validate(const hash_digest& hash, bool positive)
{
    // TODO: eliminate the double state lookup for state update.
    // TODO: the caller doesn't know the current header state.

    // This is pointer to the data section of the record row entry.
    auto record = hash_table_.find(hash);

    if (record == nullptr)
        return false;

    const auto memory = record->buffer();
    ////const auto prefix_start = memory - prefix_size_;

    // The header and height never change after the block is reachable.
    auto deserial = make_unsafe_deserializer(memory + height_offset);
    const auto height = deserial.read_4_bytes_little_endian();

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    metadata_mutex_.lock_shared();
    const auto original = deserial.read_byte();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    record = nullptr;
    const auto state = update_validation_state(original, positive);
    const auto update = [&](byte_serializer& serial)
    {
        ///////////////////////////////////////////////////////////////////////
        // Critical Section.
        unique_lock lock(metadata_mutex_);

        // Skip block header, including median_time_past metadata, and height.
        serial.skip(state_offset);
        serial.write_byte(state);
    };

    hash_table_.update(hash, update);

    // Also update the validation chaser, assumes all prior are valid.
    BITCOIN_ASSERT_MSG(valid_point_ != max_size_t, "valid point overflow");
    BITCOIN_ASSERT_MSG((height == 0 && valid_point_ == 0) ||
        (height == valid_point_ + 1), "validation out of order");

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

bool block_database::confirm(const hash_digest& hash, size_t height,
    bool block_index)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = block_index ? block_index_ : header_index_;

    // Can only confirm at the top of the given index (push).
    if (height != manager.count())
        return false;

    // TODO: eliminate the double state lookup for state update.
    // TODO: the caller doesn't know the current header state.
    auto record = hash_table_.get(read_index(height, manager));
    record->increment(prefix_size_);
    const auto state_start = record->buffer();
    auto deserial = make_unsafe_deserializer(state_start);

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    metadata_mutex_.lock_shared();
    const auto original = deserial.read_byte();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    record = nullptr;
    const auto state = update_confirmation_state(original, true, block_index);
    const auto update = [&](byte_serializer& serial)
    {
        ///////////////////////////////////////////////////////////////////////
        // Critical Section.
        unique_lock lock(metadata_mutex_);

        // Skip block header, including median_time_past metadata, and height.
        serial.skip(state_offset);
        serial.write_byte(state);
    };

    // Also increment the fork point for block-indexed confirmation.
    if (block_index && is_confirmed(state))
    {
        BITCOIN_ASSERT_MSG(fork_point_ != max_size_t, "fork point overflow");
        ++fork_point_;
    }

    const auto index = hash_table_.update(hash, update);
    push_index(index, height, manager);
    return true;
}

bool block_database::unconfirm(const hash_digest& hash, size_t height,
    bool block_index)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = block_index ? block_index_ : header_index_;

    // Can only unconfirm the top of the given index (pop).
    if (height + 1u != manager.count())
        return false;

    // TODO: eliminate the double state lookup for state update.
    // TODO: the caller already knows the current header state.
    // TODO: update isn't actually required here (index unused).
    auto record = hash_table_.get(read_index(height, manager));
    record->increment(prefix_size_);
    const auto state_start = record->buffer();
    auto deserial = make_unsafe_deserializer(state_start);

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    metadata_mutex_.lock_shared();
    const auto original = deserial.read_byte();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    record = nullptr;
    const auto state = update_confirmation_state(original, false, block_index);
    const auto update = [&](byte_serializer& serial)
    {
        ///////////////////////////////////////////////////////////////////////
        // Critical Section.
        unique_lock lock(metadata_mutex_);

        // Skip block header, including median_time_past metadata, and height.
        serial.skip(state_offset);
        serial.write_byte(state);
    };

    // Only decrement the fork point for confirmed block via header-index.
    if (!block_index && is_confirmed(original))
    {
        BITCOIN_ASSERT_MSG(fork_point_ != 0, "fork point underflow");
        --fork_point_;
    }
    else
    {
        hash_table_.update(hash, update);
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
