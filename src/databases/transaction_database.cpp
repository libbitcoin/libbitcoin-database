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
#include <bitcoin/database/databases/transaction_database.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static const auto use_wire_encoding = false;
static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto version_size = sizeof(uint32_t);
static constexpr auto locktime_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint32_t);
static constexpr auto version_lock_size = version_size + locktime_size;

// Transactions uses a hash table index, O(1).
transaction_database::transaction_database(const path& map_filename,
    size_t buckets, size_t expansion, size_t cache_capacity, mutex_ptr mutex)
  : initial_map_file_size_(slab_hash_table_header_size(buckets) +
        minimum_slabs_size),

    lookup_file_(map_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, slab_hash_table_header_size(buckets)),
    lookup_map_(lookup_header_, lookup_manager_),
    cache_(cache_capacity)
{
}

transaction_database::~transaction_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool transaction_database::create()
{
    // Resize and create require an opened file.
    if (!lookup_file_.open())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);

    if (!lookup_header_.create() ||
        !lookup_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Start files and primitives.
bool transaction_database::open()
{
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Close files.
bool transaction_database::close()
{
    return lookup_file_.close();
}

// Commit latest inserts.
void transaction_database::synchronize()
{
    lookup_manager_.sync();
}

// Flush the memory map to disk.
bool transaction_database::flush()
{
    return lookup_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

transaction_result transaction_database::get(const hash_digest& hash) const
{
    return get(hash, max_size_t);
}

transaction_result transaction_database::get(const hash_digest& hash,
    size_t fork_height) const
{
    const auto memory = lookup_map_.find(hash);
    const auto result = transaction_result(memory, hash);

    if (!result)
        return result;

    // BUGBUG: use lookup_map_ to search a set of transactions in height order,
    // returning the highest that is at or below the specified fork height.
    if (result.height() > fork_height)
        return{ nullptr };

    return result;
}

bool transaction_database::get_output(output& out_output, size_t& out_height,
    bool& out_coinbase, const output_point& point, size_t fork_height) const
{
    if (cache_.get(out_output, out_height, out_coinbase, point, fork_height))
        return true;

    const auto result = get(point.hash(), fork_height);

    // The transaction does not exist.
    if (!result)
        return false;

    out_height = result.height();
    out_coinbase = result.position() == 0;
    out_output = result.output(point.index());
    return true;
}

bool transaction_database::update(const output_point& point,
    size_t spender_height)
{
    cache_.remove(point);
    const auto slab = lookup_map_.find(point.hash());

    // The transaction does not exist.
    if (slab == nullptr)
        return false;

    const auto memory = REMAP_ADDRESS(slab);
    const auto tx_start = memory + height_size + position_size;
    auto serial = make_unsafe_serializer(tx_start);
    serial.skip(version_size + locktime_size);
    const auto outputs = serial.read_size_little_endian();
    BITCOIN_ASSERT(serial);

    // The index is not in the transaction.
    if (point.index() >= outputs)
        return false;

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < point.index(); ++output)
    {
        serial.skip(height_size + value_size);
        serial.skip(serial.read_size_little_endian());
        BITCOIN_ASSERT(serial);
    }

    // Write the spender height to the first word of the target output.
    serial.write_4_bytes_little_endian(spender_height);
    return true;
}

void transaction_database::store(size_t height, size_t position,
    const chain::transaction& tx)
{
    // Write block data.
    const auto key = tx.hash();
    const auto tx_size = tx.serialized_size(false);

    BITCOIN_ASSERT(height <= max_uint32);
    const auto hight32 = static_cast<size_t>(height);

    BITCOIN_ASSERT(position <= max_uint32);
    const auto position32 = static_cast<size_t>(position);

    BITCOIN_ASSERT(tx_size <= max_size_t - version_lock_size);
    const auto value_size = version_lock_size + static_cast<size_t>(tx_size);

    const auto write = [&](serializer<uint8_t*>& serial)
    {
        serial.write_4_bytes_little_endian(hight32);
        serial.write_4_bytes_little_endian(position32);

        // WRITE THE TX
        tx.to_data(serial, use_wire_encoding);
    };

    lookup_map_.store(key, write, value_size);
    cache_.add(tx, height);

    if (position == 0 && ((height % 100) == 0))
        LOG_DEBUG(LOG_DATABASE)
            << "Cache hit rate: " << cache_.hit_rate() << ", size: "
            << cache_.size();
}

bool transaction_database::unlink(const hash_digest& hash)
{
    cache_.remove(hash);
    return lookup_map_.unlink(hash);
}

} // namespace database
} // namespace libbitcoin
