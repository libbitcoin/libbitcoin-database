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
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::machine;

// Record format (v4):
// ----------------------------------------------------------------------------
// [ height/forks/code:4 - atomic1  ] (code if invalid)
// [ position:2          - atomic1  ] (unconfirmed sentinel, could store state)
// [ candidate:1         - atomic1  ] (candidate(1))
// [ median_time_past:4  - atomic1  ] (zero if unconfirmed)
// [ output_count:varint - const    ] (tx starts here)
// [
//   [ candidate_spent:1 - atomic2 ]
//   [ spender_height:4  - atomic2 ]  (could store candidate_spent in high bit)
//   [ value:8           - const   ]
//   [ script:varint     - const   ]
// ]...
// [ input_count:varint   - const   ]
// [
//   [ hash:32            - const  ]
//   [ index:2            - const  ]
//   [ script:varint      - const  ]
//   [ sequence:4         - const  ]
// ]...
// [ locktime:varint      - const    ]
// [ version:varint       - const    ]

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

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint16_t);
static constexpr auto candidate_size = sizeof(uint8_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);

static constexpr auto candidate_spent_size = sizeof(uint8_t);
////static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto value_size = sizeof(uint64_t);

static constexpr auto spend_size = candidate_spent_size + height_size +
    value_size;
static constexpr auto metadata_size = height_size + position_size +
    candidate_size + median_time_past_size;

static constexpr auto no_time = 0u;

// Transactions uses a hash table index, O(1).
transaction_database::transaction_database(const path& map_filename,
    size_t buckets, size_t expansion, size_t cache_capacity)
  : hash_table_file_(map_filename, expansion),
    hash_table_(hash_table_file_, buckets),
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
    if (!hash_table_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create();
}

bool transaction_database::open()
{
    return
        hash_table_file_.open() &&
        hash_table_.start();
}

void transaction_database::commit()
{
    hash_table_.commit();
}

bool transaction_database::flush() const
{
    return hash_table_file_.flush();
}

bool transaction_database::close()
{
    return hash_table_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

transaction_result transaction_database::get(file_offset offset) const
{
    // This is not guarded for an invalid offset.
    return { hash_table_.find(offset), metadata_mutex_ };
}

transaction_result transaction_database::get(const hash_digest& hash) const
{
    return { hash_table_.find(hash), metadata_mutex_ };
}

void transaction_database::get_block_metadata(const chain::transaction& tx,
    uint32_t forks, size_t fork_height) const
{
    const auto result = get(tx.hash());

    // Default values are correct for indication of not found.
    if (!result)
        return;

    const auto bip34 = chain::script::is_enabled(forks,
        machine::rule_fork::bip34_rule);

    //*************************************************************************
    // CONSENSUS: BIP30 treats a spent duplicate as if it did not exist, and
    // any duplicate of an unspent tx as invalid (with two exceptions).
    // BIP34 active renders BIP30 moot as duplicates are presumed impossible.
    //*************************************************************************
    if (!bip34 && result.is_spent(fork_height, true))
    {
        // The original tx will not be queryable independent of the block.
        // The original tx's block linkage is unbroken by accepting duplicate.
        // BIP30 exception blocks are not spent (will not be unlinked here).
        BITCOIN_ASSERT(tx.metadata.link == transaction::validation::unlinked);
        return;
    }

    const auto height = result.height();
    tx.metadata.existed = tx.metadata.link !=
        transaction::validation::unlinked;
    tx.metadata.candidate = result.candidate();
    tx.metadata.confirmed = result.position() !=
        transaction_result::unconfirmed && fork_height <= height;
    tx.metadata.link = result.link();
    tx.metadata.verified = !tx.metadata.confirmed && height == forks;
}

void transaction_database::get_pool_metadata(const chain::transaction& tx,
    uint32_t forks) const
{
    const auto result = get(tx.hash());

    // Default values presumed correct for indication of not found.
    if (!result)
        return;

    tx.metadata.existed = tx.metadata.link !=
        transaction::validation::unlinked;
    tx.metadata.candidate = result.candidate();
    tx.metadata.confirmed = result.position() !=
        transaction_result::unconfirmed;
    tx.metadata.link = result.link();
    tx.metadata.verified = !tx.metadata.confirmed && result.height() == forks;
}

// Metadata should be defaulted by caller.
bool transaction_database::get_output(const output_point& point,
    size_t fork_height, bool candidate) const
{
    auto& prevout = point.metadata;
    prevout.height = 0;
    prevout.median_time_past = 0;
    prevout.spent = false;

    // If the input is a coinbase there is no prevout to populate.
    if (point.is_null())
        return false;

    if (cache_.populate(point, fork_height))
        return true;

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

    // Find the output at the specified index for the found tx.
    prevout.cache = result.output(point.index());
    if (!prevout.cache.is_valid())
        return false;

    // Populate the output metadata.
    const auto position = result.position();
    prevout.candidate = result.candidate();
    prevout.coinbase = position == 0;
    prevout.confirmed = position != transaction_result::unconfirmed &&
        height <= fork_height;
    prevout.height = height;
    prevout.median_time_past = result.median_time_past();
    prevout.spent = prevout.cache.metadata.spent(fork_height, candidate);

    // Return is redundant with prevout.cache validity.
    return true;
}

// Store.
// ----------------------------------------------------------------------------

// Store new unconfirmed tx and set tx link metadata in any case.
bool transaction_database::store(const chain::transaction& tx, uint32_t forks)
{
    return storize(tx, forks, no_time, transaction_result::unconfirmed);
}

// Store each new tx of the unconfirmed block and set tx link metadata for all.
bool transaction_database::store(const transaction::list& transactions)
{
    for (const auto& tx: transactions)
        if (!storize(tx, rule_fork::unverified, no_time,
            transaction_result::unconfirmed))
            return false;

    return true;
}

// Store each new tx of the confirmed block and set tx link metadata for all.
bool transaction_database::store(const chain::transaction::list& transactions,
    size_t height, uint32_t median_time_past)
{
    size_t position = 0;
    for (const auto& tx: transactions)
        if (!storize(tx, height, median_time_past, position++))
            return false;

    return true;
}

// private
bool transaction_database::storize(const chain::transaction& tx, size_t height,
    uint32_t median_time_past, size_t position)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint16);

    // Assume the caller has not tested for existence (true for block update).
    if (tx.metadata.link == transaction::validation::unlinked)
    {
        // TODO: create an optimal tx.link getter for this.
        const auto result = get(tx.hash());

        if (result)
            tx.metadata.link = result.link();
    }

    // This allows address indexer to bypass indexing despite link existence.
    tx.metadata.existed = tx.metadata.link != transaction::validation::unlinked;

    // If the transaction already exists just return (with link set).
    if (tx.metadata.existed)
        return true;

    const auto writer = [&](byte_serializer& serial)
    {
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(position));
        serial.write_byte(transaction_result::candidate_false);
        serial.write_4_bytes_little_endian(median_time_past);
        tx.to_data(serial, false, true);
    };

    // Transactions are variable-sized.
    const auto size = metadata_size + tx.serialized_size(false, true);

    // Write the new transaction.
    auto next = hash_table_.allocator();
    tx.metadata.link = next.create(tx.hash(), writer, size);
    hash_table_.link(next);
    return true;
}

// Candidate/Uncandidate.
// ----------------------------------------------------------------------------

bool transaction_database::candidate(file_offset link)
{
    return candidate(link, true);
}

// private
bool transaction_database::candidate(file_offset link, bool positive)
{
    const auto result = get(link);

    if (!result || !candidize(link, positive))
        return false;

    // Spend or unspend the candidate tx's previous outputs.
    for (const auto inpoint: result)
        if (!candidate_spend(inpoint, positive))
            return false;

    return true;
}

bool transaction_database::uncandidate(file_offset link)
{
    return candidate(link, false);
}

// private
bool transaction_database::candidate_spend(const chain::output_point& point,
    bool positive)
{
    // This just simplifies calling by allowing coinbase to be included.
    if (point.is_null())
        return true;

    const auto element = hash_table_.find(point.hash());

    if (!element)
        return false;

    size_t outputs;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        deserial.skip(metadata_size);
        outputs = deserial.read_size_little_endian();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);

    // The index is not in the transaction.
    if (point.index() >= outputs)
        return false;

    const auto writer = [&](byte_serializer& serial)
    {
        serial.skip(metadata_size);
        serial.read_size_little_endian();

        // Skip outputs until the target output.
        for (auto output = 0u; output < point.index(); ++output)
        {
            serial.skip(spend_size);
            serial.skip(serial.read_size_little_endian());
        }

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_byte(positive ? transaction_result::candidate_true :
            transaction_result::candidate_false);
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(writer);
    return true;
}

// private
bool transaction_database::candidize(link_type link, bool candidate)
{
    const auto element = hash_table_.find(link);

    if (!element)
        return false;

    const auto writer = [&](byte_serializer& serial)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.skip(height_size + position_size);
        serial.write_byte(candidate ? transaction_result::candidate_true :
            transaction_result::candidate_false);
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(writer);
    return true;
}

// Confirm/Unconfirm.
// ----------------------------------------------------------------------------

bool transaction_database::confirm(const transaction::list& transactions,
    size_t height, uint32_t median_time_past)
{
    uint32_t position = 0;
    for (const auto& tx: transactions)
        if (!confirm(tx.metadata.link, height, median_time_past, position++))
            return false;

    return true;
}

bool transaction_database::confirm(file_offset link, size_t height,
    uint32_t median_time_past, size_t position)
{
    const auto result = get(link);

    if (!result)
        return false;

    // Spend or unspend the tx's previous outputs.
    for (const auto inpoint: result)
        if (!confirmed_spend(inpoint, height))
            return false;

    const auto confirmed = position != transaction_result::unconfirmed;

    // TODO: It may be more costly to populate the tx than the cache benefit.
    if (!cache_.disabled())
        cache_.add(result.transaction(), height, median_time_past, confirmed);

    // Promote the tx that already exists.
    return confirmize(link, height, median_time_past, position);
}

bool transaction_database::unconfirm(file_offset link)
{
    // The tx was verified under a now unknown chain state, so set unverified.
    return confirm(link, rule_fork::unverified, no_time,
        transaction_result::unconfirmed);
}

// private
bool transaction_database::confirmed_spend(const output_point& point,
    size_t spender_height)
{
    // This just simplifies calling by allowing coinbase to be included.
    if (point.is_null())
        return true;

    const auto unspend = (spender_height & output::validation::unspent) != 0;

    // When unspending could restore the spend to the cache, but not worth it.
    if (unspend && !cache_.disabled())
        cache_.remove(point);

    const auto element = hash_table_.find(point.hash());

    if (!element)
        return false;

    size_t outputs;
    uint32_t height;
    uint16_t position;
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        height = deserial.read_4_bytes_little_endian();
        position = deserial.read_2_bytes_little_endian();
        deserial.skip(candidate_size + median_time_past_size);
        outputs = deserial.read_size_little_endian();
        ///////////////////////////////////////////////////////////////////////
    };

    element.read(reader);

    // Limit to confirmed transactions at or below the spender height.
    if (position == transaction_result::unconfirmed || height > spender_height)
        return false;

    // The index is not in the transaction.
    if (point.index() >= outputs)
        return false;

    const auto writer = [&](byte_serializer& serial)
    {
        serial.skip(metadata_size);
        serial.read_size_little_endian();

        // Skip outputs until the target output.
        for (auto output = 0u; output < point.index(); ++output)
        {
            serial.skip(spend_size);
            serial.skip(serial.read_size_little_endian());
        }

        serial.skip(candidate_spent_size);

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_4_bytes_little_endian(spender_height);
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(writer);
    return true;
}

// private
bool transaction_database::confirmize(link_type link, size_t height,
    uint32_t median_time_past, size_t position)
{
    BITCOIN_ASSERT(height <= max_uint32);
    BITCOIN_ASSERT(position <= max_uint16);
    const auto element = hash_table_.find(link);

    if (!element)
        return false;

    const auto writer = [&](byte_serializer& serial)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(height));
        serial.write_2_bytes_little_endian(static_cast<uint16_t>(position));
        serial.write_byte(transaction_result::candidate_false);
        serial.write_4_bytes_little_endian(median_time_past);
        ///////////////////////////////////////////////////////////////////////
    };

    element.write(writer);
    return true;
}

} // namespace database
} // namespace libbitcoin
