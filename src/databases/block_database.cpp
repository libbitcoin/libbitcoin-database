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

// Record format (v4) [95 bytes]:
// Below excludes block height and tx hash indexes (arrays).
// ----------------------------------------------------------------------------
// [ state:1    - atomic1 ] (invalid, empty, stored, pooled, indexed, confirmed)
// [ header:80  - const   ]
// [ height:4   - const   ] (in any branch)
// [ checksum:4 - atomic2 ] (optional, zero if not cached, code if invalid)
// [ tx_start:4 - atomic3 ] (array index into the transaction_index, or zero)
// [ tx_count:2 - atomic3 ] (atomic with start, zero if block unpopulated)

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
static constexpr auto checksum_size = sizeof(uint32_t);
static constexpr auto tx_start_size = sizeof(uint32_t);
static constexpr auto tx_count_size = sizeof(uint16_t);
static constexpr auto confirmed_size = sizeof(uint8_t);
static const auto height_offset = header_size + median_time_past_size;
static const auto checksum_offset = height_offset + checksum_size;
static const auto block_size = header_size + median_time_past_size +
    height_size + checksum_size + tx_start_size + tx_count_size +
    confirmed_size;

static constexpr auto no_checksum = 0u;

static constexpr auto header_index_header_size = 0u;
static constexpr auto header_index_record_size = sizeof(array_index);

static constexpr auto block_index_header_size = 0u;
static constexpr auto block_index_record_size = sizeof(array_index);

static constexpr auto tx_index_header_size = 0u;
static constexpr auto tx_index_record_size = sizeof(file_offset);

// The block database keys off of block hash and has block value.
static const auto record_size = hash_table_record_size<hash_digest>(block_size);

bool block_database::is_confirmed(block_state status)
{
    return status == block_state::confirmed;
}

bool block_database::is_indexed(block_state status)
{
    return status == block_state::indexed;
}

bool block_database::is_pooled(block_state status)
{
    return status == block_state::pooled || status == block_state::indexed;
}

bool block_database::is_valid(block_state status)
{
    return is_confirmed(status) || is_indexed(status) || is_pooled(status);
}

block_state block_database::to_status(bool confirmed)
{
    return confirmed ? block_state::confirmed : block_state::pooled;
}

// Blocks uses a hash table and two array indexes, all O(1).
block_database::block_database(const path& map_filename,
    const path& header_index_filename, const path& block_index_filename,
    const path& tx_index_filename, size_t buckets, size_t expansion,
    mutex_ptr mutex)
  : initial_map_file_size_(record_hash_table_header_size(buckets) +
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

// Create.
// ----------------------------------------------------------------------------

// Initialize files and open.
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

// Startup and shutdown.
// ----------------------------------------------------------------------------

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

bool block_database::close()
{
    return
        lookup_file_.close() &&
        header_index_file_.close() &&
        block_index_file_.close() &&
        tx_index_file_.close();
}

void block_database::synchronize()
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

// Queries.
// ----------------------------------------------------------------------------

block_result block_database::get(size_t height) const
{
    if (height >= block_index_manager_.count())
        return{ tx_index_manager_ };

    const auto height32 = static_cast<uint32_t>(height);
    auto record = lookup_manager_.get(get_index(height));
    const auto prefix = REMAP_ADDRESS(record);

    // Advance the record row entry past the key and link to the record data.
    REMAP_INCREMENT(record, prefix_size);
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(record));

    // The header and height never change after the block is reachable.
    deserial.skip(checksum_offset);

    ///////////////////////////////////////////////////////////////////////////
    metadata_mutex_.lock_shared();
    const auto checksum = deserial.read_4_bytes_little_endian();
    const auto tx_start = deserial.read_4_bytes_little_endian();
    const auto tx_count = deserial.read_2_bytes_little_endian();
    const auto state = static_cast<block_state>(deserial.read_byte());
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Must be strong chain block if indexed by height.
    BITCOIN_ASSERT(is_confirmed(state));

    // HACK: back up into the record to obtain the hash/key (optimization).
    auto reader = make_unsafe_deserializer(prefix);

    // Reads are not deferred for updatable values as atomicity is required.
    return{ tx_index_manager_, record, reader.read_hash(), height32, checksum,
        tx_start, tx_count, true };
}

block_result block_database::get(const hash_digest& hash,
    bool require_confirmed) const
{
    // This is offset to the data section of the record row entry.
    const auto record = lookup_map_.find(hash);

    if (!record)
        return{ tx_index_manager_ };

    const auto memory = REMAP_ADDRESS(record);
    ////const auto prefix_start = memory - prefix_size;

    // The header and height never change after the block is reachable.
    auto deserial = make_unsafe_deserializer(memory + height_offset);
    const auto height = deserial.read_4_bytes_little_endian();

    ///////////////////////////////////////////////////////////////////////////
    metadata_mutex_.lock_shared();
    const auto checksum = deserial.read_4_bytes_little_endian();
    const auto tx_start = deserial.read_4_bytes_little_endian();
    const auto tx_count = deserial.read_2_bytes_little_endian();
    const auto state = static_cast<block_state>(deserial.read_byte());
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    if (!is_valid(state))
        return{ tx_index_manager_ };

    const auto confirmed = is_confirmed(state);

    if (require_confirmed && !confirmed)
        return{ tx_index_manager_ };

    // Reads are not deferred for updatable values as atomicity is required.
    return{ tx_index_manager_, record, hash, height, checksum, tx_start,
        tx_count, confirmed };
}

// Save each transaction offset into the transaction_index and return the index
// of the first entry. Offsets must be cached in tx metadata.
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

// Save each transaction offset into the transaction_index and return the index
// of the first entry. Offsets must be cached in short_id metadata.
array_index block_database::associate(const short_id_list& ids)
{
    BITCOIN_ASSERT_MSG(false, "not implemented");

    if (ids.empty())
        return 0;

    const auto start = tx_index_manager_.new_records(ids.size());
    const auto record = tx_index_manager_.get(start);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(record));

    for (const auto& id: ids)
    {
        // TODO: create and employ short_id type with offset metadata.
        ////const auto offset = id.validation.offset;
        ////BITCOIN_ASSERT(offset != slab_map::not_found);
        ////serial.write_8_bytes_little_endian(offset);
    }

    return start;
}

// Store.
// ----------------------------------------------------------------------------

// private
void block_database::store(const chain::header& header, size_t height,
    uint32_t checksum, array_index tx_start, size_t tx_count,
    block_state status)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);
    const auto height32 = static_cast<uint32_t>(height);

    // This creates a new header entry and is thread safe, so no locking.
    const auto write = [&](byte_serializer& serial)
    {
        // Write block header including median_time_past metadata.
        header.to_data(serial, false);
        serial.write_4_bytes_little_endian(height32);
        serial.write_4_bytes_little_endian(checksum);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
        serial.write_byte(static_cast<uint8_t>(status));
    };

    const auto index = lookup_map_.store(header.hash(), write);

    if (!is_confirmed(status))
        return;

    write_index(index, height32);
}

// This assumes the header does not exist and has no knowledge of required txs.
void block_database::store(const chain::header& header, size_t height)
{
    static constexpr auto tx_count = 0;
    store(header, height, no_checksum, 0, tx_count, block_state::empty);
}

// This assumes the header does not exist and requires tx association metadata.
void block_database::store(const chain::block& block, size_t height,
    bool confirmed)
{
    // TODO: cache block.checksum() from message.
    const auto checksum = no_checksum;
    const auto& header = block.header();
    const auto& txs = block.transactions();
    store(header, height, checksum, associate(txs), txs.size(),
        to_status(confirmed));
}

// This assumes the header does not exist and requires tx association metadata.
void block_database::store(const message::compact_block& compact,
    size_t height, bool confirmed)
{
    const auto& header = compact.header();
    const auto& ids = compact.short_ids();
    store(header, height, no_checksum, associate(ids), ids.size(),
        to_status(confirmed));
}

// Update.
// ----------------------------------------------------------------------------

////// private
////bool block_database::update(const hash_digest& hash, uint32_t checksum,
////    array_index tx_start, size_t tx_count, uint8_t status)
////{
////    BITCOIN_ASSERT(tx_start <= max_uint32);
////    BITCOIN_ASSERT(tx_count <= max_uint16);
////
////    // Update modifies metadata, requires lock for consistency and atomicity.
////    const auto update = [&](byte_serializer& serial)
////    {
////        serial.skip(header_size + height_size);
////
////        ///////////////////////////////////////////////////////////////////////
////        // Critical Section
////        unique_lock lock(metadata_mutex_);
////        serial.write_4_bytes_little_endian(checksum);
////        serial.write_4_bytes_little_endian(static_cast<uint32_t>(tx_start));
////        serial.write_2_bytes_little_endian(static_cast<uint16_t>(tx_count));
////        serial.write_byte(status);
////        ///////////////////////////////////////////////////////////////////////
////    };
////
////    // We never change the height of a block and we cannot update to confirmed.
////    const auto index = lookup_map_.update(hash, update);
////    return index != record_map::not_found;
////}
////
////bool block_database::update(const chain::block& block, bool confirmed)
////{
////    // TODO: cache block.checksum() from message.
////    const auto checksum = no_checksum;
////
////    const auto& txs = block.transactions();
////    return update(block.hash(), checksum, associate(txs), txs.size(),
////        to_status(confirmed));
////}
////
////bool block_database::update(const message::compact_block& compact,
////    bool confirmed)
////{
////    const auto& ids = compact.short_ids();
////    return update(compact.header().hash(), no_checksum, associate(ids),
////        ids.size(), to_status(confirmed));
////}

// Confirm.
// ----------------------------------------------------------------------------

bool block_database::confirm(const hash_digest& hash, bool confirm)
{
    const auto update = [&](byte_serializer& serial)
    {
        serial.skip(block_size - confirmed_size);

        ///////////////////////////////////////////////////////////////////////
        // Critical Section
        unique_lock lock(metadata_mutex_);
        serial.write_byte(static_cast<uint8_t>(to_status(confirm)));
        ///////////////////////////////////////////////////////////////////////
    };

    return lookup_map_.update(hash, update) != record_map::not_found;
}

bool block_database::unconfirm(size_t from_height)
{
    const auto count = block_index_manager_.count();

    if (from_height >= count)
        return false;

    for (auto height = from_height; height < count; ++height)
        if (!confirm(get(height).hash(), false))
            return false;

    // This is the only place where the logical index is reduced in length.
    // This will remove from the index all references at and above from_height.
    block_index_manager_.set_count(from_height);
    return true;
}

// Index.
// ----------------------------------------------------------------------------

void block_database::write_index(array_index index, array_index height)
{
    BITCOIN_ASSERT(height < max_uint32);
    BITCOIN_ASSERT(block_index_manager_.count() == height);

    block_index_manager_.new_records(1);
    const auto record = block_index_manager_.get(height);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(record));
    serial.write_4_bytes_little_endian(index);
}

array_index block_database::get_index(array_index height) const
{
    const auto record = block_index_manager_.get(height);
    return from_little_endian_unsafe<array_index>(REMAP_ADDRESS(record));
}

// The height of the highest confirmed block.
bool block_database::top(size_t& out_height) const
{
    const auto count = block_index_manager_.count();

    // Guard against no genesis block.
    if (count == 0)
        return false;

    out_height = count - 1;
    return true;
}

} // namespace database
} // namespace libbitcoin
