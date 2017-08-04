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
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::machine;

// Record format (v4):
// ----------------------------------------------------------------------------
// [ height/forks/code:4 - atomic1 ] (code if invalid)
// [ position:2          - atomic1 ] (unconfirmed sentinel, could store state)
// [ state:1             - atomic1 ] (invalid, stored, pooled, indexed, confirmed)
// [ median_time_past:4  - atomic1 ] (zero if unconfirmed)
// [ output_count:varint - const   ]
// [
//   [ index_spend:1    - atomic2 ]
//   [ spender_height:4 - atomic2 ] (could store index_spend in high bit)
//   [ value:8          - const   ]
//   [ script:varint    - const   ]
// ]...
// [ input_count:varint   - const   ]
// [
//   [ hash:32           - const  ]
//   [ index:2           - const  ]
//   [ script:varint     - const  ]
//   [ sequence:4        - const  ]
// ]...
// [ locktime:varint      - const   ]
// [ version:varint       - const   ]

// Record format (v3.3):
// ----------------------------------------------------------------------------
// [ height/forks:4         - atomic1 ]
// [ position/unconfirmed:2 - atomic1 ]
// [ median_time_past:4     - atomic1 ]
// [ output_count:varint    - const   ]
// [ [ spender_height:4 - atomic2 ][ value:8 ][ script:varint ] ]...
// [ input_count:varint     - const   ]
// [ [ hash:32 ][ index:2 ][ script:varint ][ sequence:4 ] ]...
// [ locktime:varint        - const   ]
// [ version:varint         - const   ]

static BC_CONSTEXPR auto prefix_size = slab_row<hash_digest>::prefix_size;
static constexpr auto value_size = sizeof(uint64_t);

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint16_t);
static constexpr auto state_size = sizeof(uint8_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto spender_height_value_size = height_size + value_size;
static constexpr auto metadata_size = height_size + position_size +
    state_size + median_time_past_size;

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

// Startup and shutdown.
// ----------------------------------------------------------------------------

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

bool transaction_database::open()
{
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

void transaction_database::commit()
{
    lookup_manager_.sync();
}

bool transaction_database::flush() const
{
    return lookup_file_.flush();
}

bool transaction_database::close()
{
    return lookup_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

transaction_result transaction_database::get(file_offset offset) const
{
    const auto slab = lookup_manager_.get(offset);

    if (slab == nullptr)
        return{};

    const auto memory = REMAP_ADDRESS(slab);
    auto deserial = make_unsafe_deserializer(memory);

    // The four metadata values must be atomic and mutually consistent.
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    metadata_mutex_.lock_shared();
    const auto height = deserial.read_4_bytes_little_endian();
    const auto position = deserial.read_2_bytes_little_endian();
    const auto state = static_cast<transaction_state>(deserial.read_byte());
    const auto median_time_past = deserial.read_4_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // HACK: back up into the slab to obtain the hash/key (optimization).
    auto reader = make_unsafe_deserializer(memory - prefix_size);

    // Reads are not deferred for updatable values as atomicity is required.
    return{ slab, reader.read_hash(), height, median_time_past, position,
        state, offset };
}

transaction_result transaction_database::get(const hash_digest& hash) const
{
    const auto offset = lookup_map_.offset(hash);
    return offset == slab_hash_table<hash_digest>::not_found ?
        transaction_result{} : get(offset);
}

// Metadata should be defaulted by caller.
bool transaction_database::get_output(const output_point& point,
    size_t fork_height) const
{
    auto& prevout = point.validation;
    prevout.height = 0;
    prevout.median_time_past = 0;
    prevout.spent = false;

    // If the input is a coinbase there is no prevout to populate.
    if (point.is_null())
        return false;

    // Cache does not contain spent outputs or indexed confirmation states.
    if (cache_.populate(point, fork_height))
        return true;

    // Find the tx entry.
    const auto result = get(point.hash());

    if (!result)
        return false;

    //*************************************************************************
    // CONSENSUS: The genesis block coinbase output may not be spent. This is
    // the consequence of satoshi not including it in the utxo set for block
    // database initialization. Only he knows why, probably an oversight.
    //*************************************************************************
    const auto height = result.height();
    if (height == 0)
        return false;

    const auto state = result.state();
    const auto require_confirmed = (fork_height != max_size_t);
    const auto confirmed = 
        (state == transaction_state::indexed && require_confirmed) ||
        (state == transaction_state::confirmed && height <= fork_height);

    // Guarantee confirmation state.
    if (require_confirmed && !confirmed)
        return false;

    // Find the output at the specified index for the found tx.
    prevout.cache = result.output(point.index());
    if (!prevout.cache.is_valid())
        return false;

    // Populate the output metadata.
    prevout.confirmed = confirmed;
    prevout.coinbase = result.position() == 0;

    prevout.height = height;
    prevout.median_time_past = result.median_time_past();
    prevout.spent = prevout.confirmed && prevout.cache.validation.spent(
        fork_height, require_confirmed);

    // Return is redundant with cache validity.
    return true;
}

// Store.
// ----------------------------------------------------------------------------

// False implies store corruption.
bool transaction_database::store(const chain::transaction& tx, size_t height,
    uint32_t median_time_past, size_t position, transaction_state state)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint16);
    const auto confirming = (state == transaction_state::confirmed);

    // TODO: enable promotion from any unconfirmed state to pooled.
    if (confirming)
    {
        // Confirm the tx's previous outputs.
        for (const auto& input: tx.inputs())
            if (!spend(input.previous_output(), height))
                return false;

        // Promote the tx that already exists.
        if (tx.validation.offset != slab_map::not_found)
        {
            cache_.add(tx, height, median_time_past, confirming);
            return confirm(tx.validation.offset, height, median_time_past,
                position, state);
        }
    }

    const auto write = [&](byte_serializer& serial)
    {
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(position));
        serial.write_byte(static_cast<uint8_t>(state));
        serial.write_4_bytes_little_endian(median_time_past);
        tx.to_data(serial, false);
    };

    // Write the new transaction.
    const auto size = metadata_size + tx.serialized_size(false);
    tx.validation.offset = lookup_map_.store(tx.hash(), write, size);
    cache_.add(tx, height, median_time_past, confirming);
    return true;
}

// Update.
// ----------------------------------------------------------------------------

// The output is confirmed spent, or the confirmed spend is unspent.
bool transaction_database::spend(const output_point& point,
    size_t spender_height)
{
    // This just simplifies calling by allowing coinbase to be included.
    if (point.is_null())
        return true;

    // If unspending we could restore the spend to the cache, but not worth it.
    if (spender_height != output::validation::not_spent)
        cache_.remove(point);

    auto slab = lookup_map_.find(point.hash());

    if (slab == nullptr)
        return false;

    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));

    // The three metadata values must be atomic and mutually consistent.
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    metadata_mutex_.lock_shared();
    const auto height = deserial.read_4_bytes_little_endian();
    const auto position = deserial.read_2_bytes_little_endian();
    const auto state = static_cast<transaction_state>(deserial.read_byte());
    ////const auto median_time_past = deserial.read_4_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Limit to confirmed transactions at or below the spender height.
    if (state != transaction_state::confirmed || height > spender_height)
        return false;

    auto serial = make_unsafe_serializer(REMAP_ADDRESS(slab) + metadata_size);
    const auto outputs = serial.read_size_little_endian();

    // The index is not in the transaction.
    if (point.index() >= outputs)
        return false;

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < point.index(); ++output)
    {
        serial.skip(spender_height_value_size);
        serial.skip(serial.read_size_little_endian());
        BITCOIN_ASSERT(serial);
    }

    // Write the spender height to the first word of the target output.
    serial.write_4_bytes_little_endian(spender_height);
    return true;
}

bool transaction_database::confirm(file_offset offset, size_t height,
    uint32_t median_time_past, size_t position, transaction_state state)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint16);
    const auto slab = lookup_manager_.get(offset);

    if (slab == nullptr)
        return false;

    auto serial = make_unsafe_serializer(REMAP_ADDRESS(slab));

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lock(metadata_mutex_);
    serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
    serial.write_2_bytes_little_endian(static_cast<uint16_t>(position));
    serial.write_byte(static_cast<uint8_t>(state));
    serial.write_4_bytes_little_endian(median_time_past);
    ///////////////////////////////////////////////////////////////////////////
    return true;
}

// False implies store corruption.
bool transaction_database::unconfirm(uint64_t offset)
{
    const auto tx = get(offset).transaction();
    tx.validation.offset = offset;
    return unconfirm(tx);
}

// False implies store corruption.
bool transaction_database::unconfirm(const chain::transaction& tx)
{
    static const uint32_t median_time_past = 0;
    BITCOIN_ASSERT(tx.validation.offset != slab_map::not_found);

    // TODO: optimize by not requiring full tx (get prevouts from offset).
    for (const auto& input: tx.inputs())
        if (!spend(input.previous_output(), output::validation::not_spent))
            return false;

    // The tx was verified under an unknown chain state, so set unverified.
    return confirm(tx.validation.offset, rule_fork::unverified,
        median_time_past, transaction_result::unconfirmed,
        transaction_state::pooled);
}

} // namespace database
} // namespace libbitcoin
