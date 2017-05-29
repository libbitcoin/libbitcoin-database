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
// [ state:1              - atomic1 ] (invalid, stored, pooled, indexed, confirmed)
// [ height/forks:4       - atomic1 ] (atomic with position, code if invalid)
// [ position:2           - atomic1 ] (atomic with height, could store state)
// [ output_count:varint  - const   ]
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

// Record format (v3):
// ----------------------------------------------------------------------------
// [ height/forks:4         - atomic ] (atomic with position)
// [ position/unconfirmed:2 - atomic ] (atomic with height)
// [ output_count:varint    - const  ]
// [ [ spender_height:4 - atomic ][ value:8 ][ script:varint ] ]...
// [ input_count:varint     - const  ]
// [ [ hash:32 ][ index:2 ][ script:varint ][ sequence:4 ] ]...
// [ locktime:varint        - const  ]
// [ version:varint         - const  ]

static BC_CONSTEXPR auto prefix_size = slab_row<hash_digest>::prefix_size;
static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint16_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto spender_height_value_size = height_size + value_size;
static constexpr auto metadata_size = height_size + position_size +
    median_time_past_size;

// TODO: add indexed spender flag to each output (cache?).

// Valid tx position should never reach 2^16.
const size_t transaction_database::unconfirmed = max_uint16;

bool transaction_database::is_confirmed(transaction_state status)
{
    return status == transaction_state::confirmed;
}

bool transaction_database::is_indexed(transaction_state status)
{
    return status == transaction_state::indexed;
}

bool transaction_database::is_pooled(transaction_state status)
{
    return status == transaction_state::pooled ||
        status == transaction_state::indexed;
}

bool transaction_database::is_valid(transaction_state position)
{
    return is_confirmed(position) || is_indexed(position) ||
        is_pooled(position);
}

transaction_state transaction_database::to_status(bool confirmed)
{
    return confirmed ? transaction_state::confirmed :
        transaction_state::pooled;
}

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

// private
memory_ptr transaction_database::find(const hash_digest& hash,
    size_t fork_height, bool require_confirmed) const
{
    // HACK: this implementation assumes tx hash collisions cannot happen.
    auto slab = lookup_map_.find(hash /*, fork_height, require_confirmed*/);

    if (slab == nullptr || !require_confirmed)
        return slab;

    // If position is unconfirmed then height is the forks used for validation.
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    metadata_mutex_.lock_shared();
    const auto height = deserial.read_4_bytes_little_endian();
    const auto position = deserial.read_2_bytes_little_endian();
    ////const auto median_time_past = deserial.read_4_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    const auto confirmed = (position != unconfirmed);
    return (confirmed && height > fork_height) || (require_confirmed &&
        !confirmed) ? nullptr : slab;
}

transaction_result transaction_database::get(file_offset offset) const
{
    const auto slab = lookup_manager_.get(offset);

    if (!slab)
        return{};

    const auto memory = REMAP_ADDRESS(slab);
    auto deserial = make_unsafe_deserializer(memory);

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    metadata_mutex_.lock_shared();
    const auto height = deserial.read_4_bytes_little_endian();
    const auto position = deserial.read_2_bytes_little_endian();
    const auto median_time_past = deserial.read_4_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // HACK: back up into the slab to obtain the hash/key (optimization).
    auto reader = make_unsafe_deserializer(memory - prefix_size);

    // Reads are not deferred for updatable values as atomicity is required.
    return{ slab, reader.read_hash(), height, median_time_past, position };
}

transaction_result transaction_database::get(const hash_digest& hash,
    size_t fork_height, bool require_confirmed) const
{
    // Limit search to confirmed transactions at or below the fork height.
    // Caller should set fork height to max_size_t for unconfirmed search.
    const auto slab = find(hash, fork_height, require_confirmed);

    if (!slab)
        return{};

    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));

    ///////////////////////////////////////////////////////////////////////////
    metadata_mutex_.lock_shared();
    const auto height = deserial.read_4_bytes_little_endian();
    const auto position = deserial.read_2_bytes_little_endian();
    const auto median_time_past = deserial.read_4_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Reads are not deferred for updatable values as atomicity is required.
    return{ slab, hash, height, median_time_past, position };
}

// TODO: fork_height is block index only.
bool transaction_database::get_output(output& out_output, size_t& out_height,
    uint32_t& out_median_time_past, bool& out_coinbase,
    const output_point& point, size_t fork_height,
    bool require_confirmed) const
{
    if (cache_.get(out_output, out_height, out_median_time_past, out_coinbase,
        point, fork_height, require_confirmed))
        return true;

    const auto slab = find(point.hash(), fork_height, require_confirmed);

    if (!slab)
        return false;

    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));

    ///////////////////////////////////////////////////////////////////////////
    metadata_mutex_.lock_shared();
    out_height = deserial.read_4_bytes_little_endian();
    out_coinbase = deserial.read_2_bytes_little_endian() == 0;
    out_median_time_past = deserial.read_4_bytes_little_endian();
    metadata_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Result is used only to parse the output.
    transaction_result result(slab, point.hash(), 0, 0, 0);

    // TODO: add serialized spent flag to chain::output (vs. metadata)?
    // TODO: should be able to clear spender height if spent above fork point.
    // TODO: this would simplify calling by allowing for a boolean return.
    // TODO: this would allow merging the spent by header index flag to result.
    // TODO: merge header index for require_confired and ignore otherwise.
    out_output = result.output(point.index());
    return true;
}

// Store.
// ----------------------------------------------------------------------------

file_offset transaction_database::store(const chain::transaction& tx,
    size_t height, uint32_t median_time_past, size_t position)
{
    const auto hash = tx.hash();

    // If is block tx previously identified as pooled then update the tx.
    // If confirm returns false the tx did not exist so create the tx.
    // A false pooled flag saves the cost of predictable confirm failure.
    if (position != unconfirmed && position != 0 && tx.validation.pooled)
    {
        const auto offset = confirm(hash, height, median_time_past, position);

        if (offset != slab_map::not_found)
        {
            cache_.add(tx, height, median_time_past, true);
            return offset;
        }

        // No terminate here as this is only a cache and there is no fail mode.
        // Instead this falls through and creates a new transaction.
        BITCOIN_ASSERT_MSG(false, "pooled transaction not found");
    }

    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint16);

    // If position is unconfirmed then height is validation forks.
    const auto write = [&](byte_serializer& serial)
    {
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(position));
        serial.write_4_bytes_little_endian(median_time_past);
        tx.to_data(serial, false);
    };

    const auto tx_size = tx.serialized_size(false);
    BITCOIN_ASSERT(tx_size <= max_size_t - metadata_size);
    const auto total_size = metadata_size + static_cast<size_t>(tx_size);

    const auto offset = lookup_map_.store(hash, write, total_size);
    cache_.add(tx, height, median_time_past, position != unconfirmed);

    // We report this here because its a steady interval (block announce).
    if (!cache_.disabled() && position == 0)
    {
        LOG_DEBUG(LOG_DATABASE)
            << "Output cache hit rate: " << cache_.hit_rate() << ", size: "
            << cache_.size();
    }

    return offset;
}

// Update.
// ----------------------------------------------------------------------------

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

    const auto tx_start = REMAP_ADDRESS(slab) + metadata_size;
    auto serial = make_unsafe_serializer(tx_start);
    const auto outputs = serial.read_size_little_endian();
    BITCOIN_ASSERT(serial);

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

bool transaction_database::unspend(const output_point& point)
{
    return spend(point, output::validation::not_spent);
}

file_offset transaction_database::confirm(const hash_digest& hash,
    size_t height, uint32_t median_time_past, size_t position)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint16);

    const auto update = [&](byte_serializer& serial)
    {
        ///////////////////////////////////////////////////////////////////////
        // Critical Section
        unique_lock lock(metadata_mutex_);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(position));
        serial.write_4_bytes_little_endian(median_time_past);
        ///////////////////////////////////////////////////////////////////////
    };

    return lookup_map_.update(hash, update);
}

bool transaction_database::unconfirm(const hash_digest& hash)
{
    // The transaction was verified under an unknown chain state, so we set the
    // verification forks to unverified. This will compel re-validation of the
    // unconfirmed transaction before acceptance into mempool/template queries.
    return confirm(hash, rule_fork::unverified, 0, unconfirmed) !=
        slab_map::not_found;
}

} // namespace database
} // namespace libbitcoin
