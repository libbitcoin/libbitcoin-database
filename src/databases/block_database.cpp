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
//  [ header:80   - const  ]
//  [ height:4    - const  ] (in any branch)
//  [ checksum:4  - atomic ] (zero if not cached)
//  [ tx_start:4  - atomic ] (an array index into the transaction_index)
//  [ tx_count:2  - atomic ] (atomic with start, both zeroed if empty)
//  [ confirmed:1 - atomic ] (zero if not in main branch)

// Record format (v3) [variable bytes] (median_time_past added in v3.3):
// Below excludes block height index (array).
// ----------------------------------------------------------------------------
//  [ header:80          - const ]
//  [ median_time_past:4 - const ]
//  [ height:4           - const ]
//  [ tx_count:1-2       - const ]
//  [ [ tx_hash:32 ]...  - const ]


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

static constexpr auto block_index_header_size = 0u;
static constexpr auto block_index_record_size = sizeof(array_index);

static constexpr auto tx_index_header_size = 0u;
static constexpr auto tx_index_record_size = sizeof(file_offset);

// The block database keys off of block hash and has block value.
static const auto record_size = hash_table_record_size<hash_digest>(block_size);

// Valid block indexes must not reach max_uint32.
const array_index block_database::empty = max_uint32;

enum class status : uint8_t
{
    unconfirmed = 0,
    confirmed = 1
};

inline uint8_t to_status(bool confirmed)
{
    return static_cast<uint8_t>(confirmed ? status::confirmed :
        status::unconfirmed);
}

inline bool is_confirmed(uint8_t value)
{
    return static_cast<status>(value) == status::confirmed;
}

// Blocks uses a hash table and two array indexes, all O(1).
block_database::block_database(const path& map_filename,
    const path& block_index_filename, const path& tx_index_filename,
    size_t buckets, size_t expansion, mutex_ptr mutex)
  : initial_map_file_size_(record_hash_table_header_size(buckets) +
        minimum_records_size),

    lookup_file_(map_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, record_hash_table_header_size(buckets),
        record_size),
    lookup_map_(lookup_header_, lookup_manager_),

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
        !block_index_file_.open() ||
        !tx_index_file_.open())
        return false;

    // These will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);
    block_index_file_.resize(minimum_records_size);
    tx_index_file_.resize(minimum_records_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create() ||
        !block_index_manager_.create() ||
        !tx_index_manager_.create())
        return false;

    // Should not call open after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start() &&
        block_index_manager_.start() &&
        tx_index_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool block_database::open()
{
    return
        lookup_file_.open() &&
        block_index_file_.open() &&
        tx_index_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        block_index_manager_.start() &&
        tx_index_manager_.start();
}

bool block_database::close()
{
    return
        lookup_file_.close() &&
        block_index_file_.close() &&
        tx_index_file_.close();
}

void block_database::synchronize()
{
    lookup_manager_.sync();
    block_index_manager_.sync();
    tx_index_manager_.sync();
}

bool block_database::flush() const
{
    return
        lookup_file_.flush() &&
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
    const auto confirmed = is_confirmed(deserial.read_byte());
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // HACK: back up into the record to obtain the hash/key (optimization).
    auto reader = make_unsafe_deserializer(prefix);

    // Reads are not deferred for updatable values as atomicity is required.
    return{ tx_index_manager_, record, reader.read_hash(), height32, checksum,
        tx_start, tx_count, confirmed };
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
    const auto confirmed = deserial.read_byte() != 0;
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

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
    uint32_t checksum, array_index tx_start, size_t tx_count, bool confirmed)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);
    const auto height32 = static_cast<uint32_t>(height);
    const auto tx_start32 = static_cast<uint32_t>(tx_start);
    const auto tx_count16 = static_cast<uint16_t>(tx_count);

    // Store creates new entry, and supports parallel, so no locking.
    const auto write = [&](byte_serializer& serial)
    {
        // Write block header including median_time_past metadata.
        header.to_data(serial, false);
        serial.write_4_bytes_little_endian(height32);
        serial.write_4_bytes_little_endian(checksum);
        serial.write_4_bytes_little_endian(tx_start32);
        serial.write_2_bytes_little_endian(tx_count16);
        serial.write_byte(to_status(confirmed));
    };

    const auto index = lookup_map_.store(header.hash(), write);

    if (!confirmed)
        return;

    // TODO: consider returning the index vs. setting it here.
    write_index(index, height32);
}

void block_database::store(const chain::header& header, size_t height)
{
    static constexpr auto tx_count = 0;
    store(header, height, no_checksum, 0, tx_count, false);
}

void block_database::store(const chain::block& block, size_t height,
    bool confirmed)
{
    // TODO: cache block.checksum() from message.
    const auto checksum = no_checksum;
    const auto& header = block.header();
    const auto& txs = block.transactions();
    store(header, height, checksum, associate(txs), txs.size(), confirmed);
}

void block_database::store(const message::compact_block& compact,
    size_t height, bool confirmed)
{
    const auto& header = compact.header();
    const auto& ids = compact.short_ids();
    store(header, height, no_checksum, associate(ids), ids.size(), confirmed);
}

// Update.
// ----------------------------------------------------------------------------

// private
bool block_database::update(const hash_digest& hash, size_t height,
    uint32_t checksum, array_index tx_start, size_t tx_count, bool confirmed)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(tx_start <= max_uint32);
    BITCOIN_ASSERT(tx_count <= max_uint16);
    const auto height32 = static_cast<uint32_t>(height);
    const auto tx_start32 = static_cast<uint32_t>(tx_start);
    const auto tx_count16 = static_cast<uint16_t>(tx_count);

    // Update modifies metadata, requiring lock for atomicity.
    const auto update = [&](byte_serializer& serial)
    {
        serial.skip(checksum_offset);

        ///////////////////////////////////////////////////////////////////////
        // Critical Section
        unique_lock lock(metadata_mutex_);
        serial.write_4_bytes_little_endian(checksum);
        serial.write_4_bytes_little_endian(tx_start32);
        serial.write_2_bytes_little_endian(tx_count16);
        serial.write_byte(to_status(confirmed));
        ///////////////////////////////////////////////////////////////////////
    };

    const auto index = lookup_map_.update(hash, update);

    if (index == record_map::not_found)
        return false;

    if (!confirmed)
        return true;

    // TODO: consider returning the index vs. setting it here.
    write_index(index, height32);
    return true;
}

bool block_database::update(const chain::block& block, size_t height,
    bool confirmed)
{
    // TODO: cache block.checksum() from message.
    const auto checksum = no_checksum;

    const auto& txs = block.transactions();
    return update(block.hash(), height, checksum, associate(txs), txs.size(),
        confirmed);
}

bool block_database::update(const message::compact_block& compact,
    size_t height, bool confirmed)
{
    const auto& ids = compact.short_ids();
    return update(compact.header().hash(), height, no_checksum, associate(ids),
        ids.size(), confirmed);
}

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
        serial.write_byte(to_status(confirm));
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

    // This will remove from the index all references at and above from_height.
    block_index_manager_.set_count(from_height);
    return true;
}

// Parallel.
// ----------------------------------------------------------------------------
// TODO: consider updating indexes only when have contiguous confirmed blocks.
// This would allow us to update in parallel while supporting query against any
// block or transaction regardless of gaps. So we drop blocks as they arrive
// under a milestone, and as the current next top arrives we index it and all
// blocks that exist after it in the header chain.
// How do we find them? Use the block index and limit top to contiguous.
// This solves the parallel import problem of spend updating!

// Safe for parallel write.
bool block_database::exists(size_t height) const
{
    return height < block_index_manager_.count() && get_index(height) != empty;
}

// Check for gaps following parallel write (i.e. restart).
bool block_database::gaps(heights& out_gaps) const
{
    const auto count = block_index_manager_.count();

    for (size_t height = 0; height < count; ++height)
        if (get_index(height) == empty)
            out_gaps.push_back(height);

    return true;
}

// This is necessary for parallel import, as gaps are created.
void block_database::zeroize(array_index first, array_index count)
{
    for (auto index = first; index < (first + count); ++index)
    {
        const auto record = block_index_manager_.get(index);
        auto serial = make_unsafe_serializer(REMAP_ADDRESS(record));
        serial.write_4_bytes_little_endian(empty);
    }
}

// The lock is necessary for parallel import and otherwise inconsequential.
void block_database::write_index(array_index index, array_index height)
{
    BITCOIN_ASSERT(height < max_uint32);
    const auto new_count = height + 1;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    index_mutex_.lock_upgrade();

    // Guard index_manager to prevent interim count increase.
    const auto initial_count = block_index_manager_.count();

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    index_mutex_.unlock_upgrade_and_lock();

    // Guard write to prevent overwriting preceding height write.
    if (new_count > initial_count)
    {
        const auto create_count = new_count - initial_count;
        block_index_manager_.new_records(create_count);
        zeroize(initial_count, create_count - 1);
    }

    // Guard write to prevent subsequent zeroize from erasing.
    const auto record = block_index_manager_.get(height);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(record));
    serial.write_4_bytes_little_endian(index);

    index_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

array_index block_database::get_index(array_index height) const
{
    const auto record = block_index_manager_.get(height);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(index_mutex_);
    return from_little_endian_unsafe<array_index>(REMAP_ADDRESS(record));
    ///////////////////////////////////////////////////////////////////////////
}

// The height of the highest existing block, independent of gaps.
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
