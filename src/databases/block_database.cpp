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
#include <bitcoin/database/result/block_result.hpp>

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

static BC_CONSTEXPR auto prefix_size = record_row<hash_digest>::prefix_size;

static const auto header_size = header::satoshi_fixed_size();
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto state_size = sizeof(uint8_t);
static constexpr auto checksum_size = sizeof(uint32_t);
static constexpr auto tx_start_size = sizeof(uint32_t);
static constexpr auto tx_count_size = sizeof(uint16_t);
static const auto height_offset = header_size + median_time_past_size;
static const auto state_offset = height_offset + height_size;
static const auto block_size = header_size + median_time_past_size +
    height_size + state_size + checksum_size + tx_start_size + tx_count_size;

static constexpr auto header_index_header_size = 0u;
static constexpr auto header_index_record_size = sizeof(array_index);
static constexpr auto block_index_header_size = 0u;
static constexpr auto block_index_record_size = sizeof(array_index);
static constexpr auto tx_index_header_size = 0u;
static constexpr auto tx_index_record_size = sizeof(file_offset);

// The block database keys off of block hash and has block value.
static const auto record_size = hash_table_record_size<hash_digest>(block_size);

// Placeholder for unimplemented checksum caching.
static constexpr auto no_checksum = 0u;

// Blocks uses a hash table and two array indexes, all O(1).
block_database::block_database(const path& map_filename,
    const path& header_index_filename, const path& block_index_filename,
    const path& tx_index_filename, size_t buckets, size_t expansion,
    mutex_ptr mutex)
  : fork_point_(0),
    initial_map_file_size_(record_hash_table_header_size(buckets) +
        minimum_records_size),

    lookup_file_(map_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, record_hash_table_header_size(buckets),
        record_size),
    lookup_map_(lookup_header_, lookup_manager_),

    header_index_file_(header_index_filename, mutex, expansion),
    header_index_manager_(header_index_file_, header_index_header_size,
        block_index_record_size),

    block_index_file_(block_index_filename, mutex, expansion),
    block_index_manager_(block_index_file_, block_index_header_size,
        block_index_record_size),

    tx_index_file_(tx_index_filename, mutex, expansion),
    tx_index_manager_(tx_index_file_, tx_index_header_size,
        tx_index_record_size)
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
    // Resize and create require an opened file.
    if (!lookup_file_.open() ||
        !header_index_file_.open() ||
        !block_index_file_.open() ||
        !tx_index_file_.open())
        return false;

    // These will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);
    header_index_file_.resize(minimum_records_size);
    block_index_file_.resize(minimum_records_size);
    tx_index_file_.resize(minimum_records_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create() ||
        !header_index_manager_.create() ||
        !block_index_manager_.create() ||
        !tx_index_manager_.create())
        return false;

    // Should not call open after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start() &&
        header_index_manager_.start() &&
        block_index_manager_.start() &&
        tx_index_manager_.start();
}

bool block_database::open()
{
    return
        lookup_file_.open() &&
        header_index_file_.open() &&
        block_index_file_.open() &&
        tx_index_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        header_index_manager_.start() &&
        block_index_manager_.start() &&
        tx_index_manager_.start();
}

void block_database::commit()
{
    lookup_manager_.sync();
    header_index_manager_.sync();
    block_index_manager_.sync();
    tx_index_manager_.sync();
}

bool block_database::flush() const
{
    return
        lookup_file_.flush() &&
        header_index_file_.flush() &&
        block_index_file_.flush() &&
        tx_index_file_.flush();
}

bool block_database::close()
{
    return
        lookup_file_.close() &&
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

bool block_database::top(size_t& out_height, bool block_index) const
{
    auto& manager = block_index ? block_index_manager_ : header_index_manager_;
    return read_top(out_height, manager);
}

block_result block_database::get(size_t height, bool block_index) const
{
    auto& manager = block_index ? block_index_manager_ : header_index_manager_;

    if (height >= manager.count())
        return{ tx_index_manager_ };

    auto record = lookup_manager_.get(read_index(height, manager));
    const auto prefix = REMAP_ADDRESS(record);

    // Advance the record row entry past the key and link to the record data.
    REMAP_INCREMENT(record, prefix_size);
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(record));

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
    return{ tx_index_manager_, record, reader.read_hash(),
        static_cast<uint32_t>(height), checksum, tx_start, tx_count, state };
}

// Returns any state, including invalid and empty.
block_result block_database::get(const hash_digest& hash) const
{
    // This is pointer to the data section of the record row entry.
    const auto record = lookup_map_.find(hash);

    if (record == nullptr)
        return{ tx_index_manager_ };

    const auto memory = REMAP_ADDRESS(record);
    ////const auto prefix_start = memory - prefix_size;

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
    return{ tx_index_manager_, record, hash, height, checksum, tx_start,
        tx_count, state };
}

// Store.
// ----------------------------------------------------------------------------

// private
void block_database::store(const chain::header& header, size_t height,
    uint32_t checksum, array_index tx_start, size_t tx_count, uint8_t state)
{
    auto& manager = is_confirmed(state) ? block_index_manager_ :
        header_index_manager_;

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

    const auto index = lookup_map_.store(header.hash(), write);

    if (is_confirmed(state) || is_indexed(state))
        push_index(index, height, manager);
}

// A header creation does not move the fork point (not a reorg).
void block_database::store(const chain::header& header, size_t height)
{
    // Initially store header as indexed, pending download (the top header).
    static const auto state = block_state::indexed | block_state::pending;

    // The header/block already exists, promote from pooled to indexed.
    if (header.validation.pooled)
    {
        confirm(header.hash(), height, false);
        return;
    }

    store(header, height, no_checksum, 0, 0, state);
}

// This creates a new store entry even if a previous existed.
// A block creation does not move the fork point (not a reorg).
void block_database::store(const chain::block& block, size_t height)
{
    // Initially store block as confirmed-valid (the top block).
    static const auto state = block_state::confirmed | block_state::valid;

    const auto& header = block.header();
    const auto& txs = block.transactions();
    store(header, height, no_checksum, associate(txs), txs.size(), state);
}

array_index block_database::associate(const transaction::list& transactions)
{
    if (transactions.empty())
        return 0;

    const auto start = tx_index_manager_.new_records(transactions.size());
    const auto record = tx_index_manager_.get(start);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(record));

    for (const auto& tx: transactions)
    {
        const auto offset = tx.validation.offset;
        BITCOIN_ASSERT(offset != record_map::not_found);
        serial.write_8_bytes_little_endian(offset);
    }

    return start;
}

// Update.
// ----------------------------------------------------------------------------
// These are used to atomically update values 

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

    // Preserve the validation state (header-indexed blocks can be pending).
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
    auto& manager = block_index ? block_index_manager_ : header_index_manager_;

    // Can only confirm at the top of the given index (push).
    if (height != manager.count())
        return false;

    // TODO: eliminate the double state lookup for state update.
    // TODO: the caller doesn't know the current header state.
    auto record = lookup_manager_.get(read_index(height, manager));
    REMAP_INCREMENT(record, prefix_size);
    const auto state_start = REMAP_ADDRESS(record);
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

    const auto index = lookup_map_.update(hash, update);
    push_index(index, height, manager);
    return true;
}

bool block_database::unconfirm(const hash_digest& hash, size_t height,
    bool block_index)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = block_index ? block_index_manager_ : header_index_manager_;

    // Can only unconfirm the top of the given index (pop).
    if (height + 1u != manager.count())
        return false;

    // TODO: eliminate the double state lookup for state update.
    // TODO: the caller already knows the current header state.
    // TODO: update isn't actually required here (index unused).
    auto record = lookup_manager_.get(read_index(height, manager));
    REMAP_INCREMENT(record, prefix_size);
    const auto state_start = REMAP_ADDRESS(record);
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
        lookup_map_.update(hash, update);
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

array_index block_database::read_index(size_t height,
    const record_manager& manager) const
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height < manager.count());

    const auto height32 = static_cast<uint32_t>(height);
    const auto record = manager.get(height32);
    return from_little_endian_unsafe<array_index>(REMAP_ADDRESS(record));
}

void block_database::pop_index(size_t height, record_manager& manager)
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height + 1u == manager.count());

    const auto height32 = static_cast<uint32_t>(height);
    manager.set_count(height32);
}

void block_database::push_index(array_index index, size_t height,
    record_manager& manager)
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height == manager.count());

    manager.new_records(1);
    const auto height32 = static_cast<uint32_t>(height);
    const auto record = manager.get(height32);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(record));
    serial.write_4_bytes_little_endian(index);
}

} // namespace database
} // namespace libbitcoin
