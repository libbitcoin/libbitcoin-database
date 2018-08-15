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
#include <bitcoin/database/block_state.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/result/block_result.hpp>

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

// Total size of block header and metadta storage.
static const auto block_size = header_size + median_time_past_size +
    height_size + state_size + checksum_size + tx_start_size + tx_count_size;

// Blocks uses a hash table and two array indexes, all O(1).
// The block database keys off of block hash and has block value.
block_database::block_database(const path& map_filename,
    const path& candidate_index_filename, const path& confirmed_index_filename,
    const path& tx_index_filename, size_t buckets, size_t expansion)
  : hash_table_file_(map_filename, expansion),
    hash_table_(hash_table_file_, buckets, block_size),

    // Array storage.
    candidate_index_file_(candidate_index_filename, expansion),
    candidate_index_(candidate_index_file_, 0, sizeof(link_type)),

    // Array storage.
    confirmed_index_file_(confirmed_index_filename, expansion),
    confirmed_index_(confirmed_index_file_, 0, sizeof(link_type)),

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
        !candidate_index_file_.open() ||
        !confirmed_index_file_.open() ||
        !tx_index_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create() &&
        candidate_index_.create() &&
        confirmed_index_.create() &&
        tx_index_.create();
}

bool block_database::open()
{
    return
        hash_table_file_.open() &&
        candidate_index_file_.open() &&
        confirmed_index_file_.open() &&
        tx_index_file_.open() &&

        hash_table_.start() &&
        candidate_index_.start() &&
        confirmed_index_.start() &&
        tx_index_.start();
}

void block_database::commit()
{
    hash_table_.commit();
    candidate_index_.commit();
    confirmed_index_.commit();
    tx_index_.commit();
}

bool block_database::flush() const
{
    return
        hash_table_file_.flush() &&
        candidate_index_file_.flush() &&
        confirmed_index_file_.flush() &&
        tx_index_file_.flush();
}

bool block_database::close()
{
    return
        hash_table_file_.close() &&
        candidate_index_file_.close() &&
        confirmed_index_file_.close() &&
        tx_index_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

bool block_database::top(size_t& out_height, bool candidate) const
{
    auto& manager = candidate ? candidate_index_ : confirmed_index_;
    return read_top(out_height, manager);
}

block_result block_database::get(size_t height, bool candidate) const
{
    auto& manager = candidate ? candidate_index_ : confirmed_index_;
    const auto link = height < manager.count() ? read_index(height, manager) :
        hash_table_.not_found;

    return
    {
        hash_table_.find(link),
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

void block_database::get_header_metadata(const chain::header& header) const
{
    const auto result = get(header.hash());

    // Default values presumed correct for indication of not found.
    if (!result)
        return;

    const auto state = result.state();
    const auto height = result.height();

    header.metadata.error = result.error();
    header.metadata.exists = true;
    header.metadata.populated = result.transaction_count() != 0;
    header.metadata.validated = is_valid(state) || is_failed(state);
    header.metadata.candidate = is_candidate(state);
    header.metadata.confirmed = is_confirmed(state);
}

// Store.
// ----------------------------------------------------------------------------

// private
void block_database::store(const chain::header& header, size_t height,
    uint32_t median_time_past, uint32_t checksum, link_type tx_start,
    size_t tx_count, uint8_t state)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);
    BITCOIN_ASSERT(!header.metadata.exists);

    const auto writer = [&](byte_serializer& serial)
    {
        header.to_data(serial, false);
        serial.write_4_bytes_little_endian(median_time_past);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_byte(state);
        serial.write_4_bytes_little_endian(checksum);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
    };

    auto next = hash_table_.allocator();
    const auto link = next.create(header.hash(), writer);
    hash_table_.link(next);
}

void block_database::store(const chain::header& header, size_t height,
    uint32_t median_time_past)
{
    static constexpr auto tx_start = 0u;
    static constexpr auto tx_count = 0u;
    static constexpr auto no_checksum = 0u;

    // New headers are only accepted in the candidate state.
    store(header, height, median_time_past, no_checksum, tx_start, tx_count,
        block_state::candidate);
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
        serial.write_8_bytes_little_endian(tx.metadata.link);

    return start;
}

// Update.
// ----------------------------------------------------------------------------
// These are used to atomically update metadata.

// Populate transaction references, state is unchanged.
bool block_database::update(const chain::block& block)
{
    auto element = hash_table_.find(block.hash());

    if (!element)
        return false;

    const auto& txs = block.transactions();
    const auto tx_start = associate(txs);
    const auto tx_count = txs.size();

    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);

    const auto updater = [&](byte_serializer& serial)
    {
        serial.skip(transactions_offset);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(updater);
    return true;
}

static uint8_t update_validation_state(uint8_t original, bool positive)
{
    // May only validate or invalidate an unvalidated block.
    BITCOIN_ASSERT(!is_failed(original) && !is_valid(original));

    // Preserve the confirmation state.
    const auto confirmation_state = original & block_state::confirmations;
    const auto validation_state = positive ? block_state::valid :
        block_state::failed;

    // Merge the new validation state with existing confirmation state.
    return confirmation_state | validation_state;
}

// Promote unvalidated block to valid|invalid based on error value.
bool block_database::validate(const hash_digest& hash, const code& error)
{
    auto element = hash_table_.find(hash);

    if (!element)
        return false;

    uint8_t state;
    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(state_offset);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        state = deserial.read_byte();
        ///////////////////////////////////////////////////////////////////////
    };

    const auto updater = [&](byte_serializer& serial)
    {
        serial.skip(state_offset);
        const auto updated = update_validation_state(state, !error);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_byte(updated);

        // Do not overwrite checksum with error code unless block is invalid.
        if (error)
            serial.write_4_bytes_little_endian(error.value());
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    element.write(updater);
    return true;
}

static uint8_t update_confirmation_state(uint8_t original, bool positive,
    bool candidate)
{
    // May only confirm a valid block.
    BITCOIN_ASSERT(!positive || candidate || is_valid(original));

    // May only unconfirm a confirmed block.
    BITCOIN_ASSERT(positive || candidate || is_confirmed(original));

    // May only candidate an unfailed block.
    BITCOIN_ASSERT(!positive || !candidate || !is_failed(original));

    // May only uncandidate a candidate header.
    BITCOIN_ASSERT(positive || !candidate || is_candidate(original));

    // Preserve the validation state (header-indexed blocks can be pent).
    const auto validation_state = original & block_state::validations;
    const auto positive_state = candidate ? block_state::candidate :
        block_state::confirmed;

    // Deconfirmation is always directly to the pooled state.
    const auto confirmation_state = positive ? positive_state :
        block_state::missing;

    // Merge the new confirmation state with existing validation state.
    return confirmation_state | validation_state;
}

uint8_t block_database::index(const_element& element, bool positive,
    bool candidate)
{
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

    uint8_t updated;
    const auto updater = [&](byte_serializer& serial)
    {
        serial.skip(state_offset);
        updated = update_confirmation_state(original, positive, candidate);

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_byte(updated);
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);
    element.write(updater);
    return positive ? updated : original;
}

bool block_database::index(const hash_digest& hash, size_t height,
    bool candidate)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = candidate ? candidate_index_ : confirmed_index_;

    // Can only add to the top of an index (push).
    if (height != manager.count())
        return false;

    auto element = hash_table_.find(hash);

    if (!element)
        return false;

    const auto updated = index(element, true, candidate);
    push_index(element.link(), height, manager);
    return true;
}

bool block_database::unindex(const hash_digest& hash, size_t height,
    bool candidate)
{
    BITCOIN_ASSERT(height != max_uint32);
    auto& manager = candidate ? candidate_index_ : confirmed_index_;

    // Can only remove from the top of an index (push).
    if (height + 1u != manager.count())
        return false;

    // Unconfirmation implies that block is indexed, so use index.
    auto element = hash_table_.find(read_index(height, manager));

    if (!element)
        return false;

    const auto original = index(element, false, candidate);
    pop_index(height, manager);
    return true;
}

// Index Utilities.
// ----------------------------------------------------------------------------

bool block_database::read_top(size_t& out_height,
    const manager_type& manager) const
{
    const auto count = manager.count();

    // Guard against no genesis block.
    if (count == 0)
        return false;

    out_height = count - 1;
    return true;
}

block_database::link_type block_database::read_index(size_t height,
    const manager_type& manager) const
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height < manager.count());

    const auto height32 = static_cast<uint32_t>(height);
    const auto record = manager.get(height32);
    return from_little_endian_unsafe<link_type>(record->buffer());
}

void block_database::pop_index(size_t height, manager_type& manager)
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(height + 1u == manager.count());

    const auto height32 = static_cast<uint32_t>(height);
    manager.set_count(height32);
}

void block_database::push_index(link_type index, size_t height,
    manager_type& manager)
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
