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
#include <bitcoin/database/databases/transaction_database.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::machine;

static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto version_size = sizeof(uint32_t);
static constexpr auto locktime_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint32_t);
static constexpr auto version_lock_size = version_size + locktime_size;

const size_t transaction_database::unconfirmed = max_uint32;

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
bool transaction_database::flush() const
{
    return lookup_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

memory_ptr transaction_database::find(const hash_digest& hash,
    size_t fork_height, bool require_confirmed) const
{
    //*************************************************************************
    // CONSENSUS: This simplified implementation does not allow the possibility
    // of a matching tx hash above the fork height or the existence of both
    // unconfirmed and confirmed transactions with the same hash. This is an
    // assumption of the impossibility of hash collisions, which is incorrect
    // but consistent with the current satoshi implementation. This method
    // encapsulates that assumption which can therefore be fixed in one place.
    //*************************************************************************
    auto slab = lookup_map_.find(hash /*, fork_height, require_confirmed*/);

    if (slab == nullptr || !require_confirmed)
        return slab;

    const auto memory = REMAP_ADDRESS(slab);
    auto deserial = make_unsafe_deserializer(memory);

    // Read the height and position.
    // If position is unconfirmed then height is the forks used for validation.
    const size_t height = deserial.read_4_bytes_little_endian();
    const size_t position = deserial.read_4_bytes_little_endian();

    return (height > fork_height) ||
        (require_confirmed && position == unconfirmed) ?
        nullptr : slab;
}

transaction_result transaction_database::get(const hash_digest& hash,
    size_t fork_height, bool require_confirmed) const
{
    // Limit search to confirmed transactions at or below the fork height.
    // Caller should set fork height to max_size_t for unconfirmed search.
    const auto slab = find(hash, fork_height, require_confirmed);

    // Returns an invalid result if not found.
    return transaction_result(slab, hash);
}

bool transaction_database::get_output(output& out_output, size_t& out_height,
    bool& out_coinbase, const output_point& point, size_t fork_height,
    bool require_confirmed) const
{
    if (cache_.get(out_output, out_height, out_coinbase, point, fork_height,
        require_confirmed))
        return true;

    const auto hash = point.hash();
    const auto slab = find(hash, fork_height, require_confirmed);

    // The transaction does not exist at/below fork with matching confirmation.
    if (!slab)
        return false;

    transaction_result result (slab, hash);
    out_height = result.height();
    out_coinbase = result.position() == 0;
    out_output = result.output(point.index());
    return true;
}

void transaction_database::store(const chain::transaction& tx,
    size_t height, size_t position)
{
    const auto hash = tx.hash();

    // If is block tx previously identified as pooled then update the tx.
    // If confirm returns false the tx did not exist so create the tx.
    // A false pooled flag saves the cost of predictable confirm failure.
    if (position != unconfirmed && position != 0 && tx.validation.pooled)
    {
        if (confirm(hash, height, position))
        {
            cache_.add(tx, height, true);
            return;
        }

        // No terminate here as this is only a cache and there is no fail mode.
        // Instead this falls through and creates a new transaction.
        BITCOIN_ASSERT_MSG(false, "pooled transaction not found");
    }

    // Create the transaction.
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint32);

    // Unconfirmed txs: position is unconfirmed and height is validation forks.
    const auto write = [&](serializer<uint8_t*>& serial)
    {
        serial.write_4_bytes_little_endian(static_cast<size_t>(height));
        serial.write_4_bytes_little_endian(static_cast<size_t>(position));

        // WRITE THE TX
        tx.to_data(serial, false);
    };

    const auto tx_size = tx.serialized_size(false);
    BITCOIN_ASSERT(tx_size <= max_size_t - version_lock_size);
    const auto value_size = version_lock_size + static_cast<size_t>(tx_size);

    // Create slab for the new tx instance.
    lookup_map_.store(hash, write, value_size);
    cache_.add(tx, height, position != unconfirmed);

    // We report theis here because its a steady interval (block announce).
    if (!cache_.disabled() && position == 0)
    {
        LOG_DEBUG(LOG_DATABASE)
            << "Output cache hit rate: " << cache_.hit_rate() << ", size: "
            << cache_.size();
    }
}

bool transaction_database::spend(const output_point& point,
    size_t spender_height)
{
    // If unspent we could restore the spend to the cache, but not worth it.
    if (spender_height != output::validation::not_spent)
        cache_.remove(point);

    // Limit search to confirmed transactions at or below the spender height,
    // since a spender cannot spend above its own height.
    // Transactions are not marked as spent unless the spender is confirmed.
    // This is consistent with support for unconfirmed double spends.
    const auto slab = find(point.hash(), spender_height, true);

    // The transaction is not exist as confirmed at or below the height.
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

bool transaction_database::unspend(const output_point& point)
{
    return spend(point, output::validation::not_spent);
}

bool transaction_database::confirm(const hash_digest& hash, size_t height,
    size_t position)
{
    const auto slab = find(hash, height, false);

    // The transaction does not exist at or below the height.
    if (slab == nullptr)
        return false;

    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint32);

    const auto memory = REMAP_ADDRESS(slab);
    auto serial = make_unsafe_serializer(memory);
    serial.write_4_bytes_little_endian(static_cast<size_t>(height));
    serial.write_4_bytes_little_endian(static_cast<size_t>(position));
    return true;
}

bool transaction_database::unconfirm(const hash_digest& hash)
{
    // The transaction was verified under an unknown chain state, so we set the
    // verification forks to unverified. This will compel re-validation of the
    // unconfirmed transaction before acceptance into mempool/template queries.
    return confirm(hash, rule_fork::unverified, unconfirmed);
}

} // namespace database
} // namespace libbitcoin
