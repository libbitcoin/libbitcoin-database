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
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace boost::filesystem;

static const auto use_wire_encoding = false;
static constexpr size_t value_size = sizeof(uint64_t);
static constexpr size_t height_size = sizeof(uint32_t);
static constexpr size_t version_size = sizeof(uint32_t);
static constexpr size_t locktime_size = sizeof(uint32_t);
static constexpr size_t position_size = sizeof(uint32_t);
static constexpr size_t version_lock_size = version_size + locktime_size;

BC_CONSTEXPR size_t number_buckets = 100000000;
BC_CONSTEXPR size_t header_size = slab_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_map_file_size = header_size + minimum_slabs_size;

transaction_database::transaction_database(const path& map_filename,
    std::shared_ptr<shared_mutex> mutex)
  : lookup_file_(map_filename, mutex), 
    lookup_header_(lookup_file_, number_buckets),
    lookup_manager_(lookup_file_, header_size),
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
    // Resize and create require a started file.
    if (!lookup_file_.open())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size);

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

// Queries.
// ----------------------------------------------------------------------------

transaction_result transaction_database::get(const hash_digest& hash,
    size_t /* fork_height */) const
{
    // TODO: use lookup_map_ to search a set of transactions in height order,
    // returning the highest that is at or below the specified fork height.
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

void transaction_database::sync()
{
    lookup_manager_.sync();
}

} // namespace database
} // namespace libbitcoin
