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
    size_t buckets, size_t expansion, mutex_ptr mutex)
  : initial_map_file_size_(slab_hash_table_header_size(buckets) +
        minimum_slabs_size),

    lookup_file_(map_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, slab_hash_table_header_size(buckets)),
    lookup_map_(lookup_header_, lookup_manager_)
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
    size_t /*DEBUG_ONLY(fork_height)*/) const
{
    // TODO: use lookup_map_ to search a set of transactions in height order,
    // returning the highest that is at or below the specified fork height.
    // Short-circuit the search if fork_height is max_size_t (just get first).
    ////BITCOIN_ASSERT_MSG(fork_height == max_size_t, "not implemented");

    const auto memory = lookup_map_.find(hash);
    return transaction_result(memory, hash);
}

bool transaction_database::update(const output_point& point,
    size_t spender_height)
{
    const auto slab = lookup_map_.find(point.hash());
    const auto memory = REMAP_ADDRESS(slab);
    const auto tx_start = memory + height_size + position_size;
    auto serial = make_unsafe_serializer(tx_start);
    serial.skip(version_size + locktime_size);
    const auto outputs = serial.read_size_little_endian();
    BITCOIN_ASSERT(serial);

    if (point.index() >= outputs)
        return false;

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < point.index(); ++output)
    {
        serial.skip(height_size);
        serial.skip(value_size);
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

    const auto write = [&hight32, &position32, &tx](memory_ptr data)
    {
        auto serial = make_unsafe_serializer(REMAP_ADDRESS(data));
        serial.write_4_bytes_little_endian(hight32);
        serial.write_4_bytes_little_endian(position32);

        // WRITE THE TX
        serial.write_bytes(tx.to_data(use_wire_encoding));
    };

    lookup_map_.store(key, write, value_size);
}

bool transaction_database::unlink(const hash_digest& hash)
{
    return lookup_map_.unlink(hash);
}

} // namespace database
} // namespace libbitcoin
