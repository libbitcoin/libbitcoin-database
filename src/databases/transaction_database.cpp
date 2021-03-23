/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::system;
using namespace bc::system::chain;
using namespace bc::system::machine;

// Record format (v4):
// ----------------------------------------------------------------------------
// [ height/forks/code:4 - atomic1  ] (code if invalid)
// [ position:2          - atomic1  ] (unconfirmed/deconfirmed sentinel, could store state)
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
    size_t table_minimum, uint32_t buckets, size_t expansion,
    size_t cache_capacity)
  : hash_table_file_(map_filename, table_minimum, expansion),
    hash_table_(hash_table_file_, buckets),
    cache_(cache_capacity)
{
    // TODO: C4267: 'argument': conversion from 'size_t' to 'Index', possible loss of data.
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

transaction_result transaction_database::get(file_offset link) const
{
    // This is not guarded for an invalid offset.
    return { hash_table_.get(link), metadata_mutex_ };
}

transaction_result transaction_database::get(const hash_digest& hash) const
{
    return { hash_table_.find(hash), metadata_mutex_ };
}

void transaction_database::get_block_metadata(const chain::transaction& tx,
    uint32_t forks, size_t fork_height) const
{
    static const auto unlinked = transaction::validation::unlinked;
    static const auto unconfirmed = transaction_result::unconfirmed;
    static const auto deconfirmed = transaction_result::deconfirmed;
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
    if (!bip34 && result.is_candidate_spent(fork_height))
    {
        // The original tx will not be queryable independent of the block.
        // The original tx's block linkage is unbroken by accepting duplicate.
        // BIP30 exception blocks are not spent (will not be unlinked here).
        BITCOIN_ASSERT(tx.metadata.link == unlinked);
        return;
    }

    const auto height = result.height();
    const auto is_linked = result.link() != unlinked;
    const auto has_position = result.position() != unconfirmed &&
        result.position() != deconfirmed;
    const auto is_confirmed = has_position && fork_height <= height;
    const auto is_cataloged =  result.position() != unconfirmed &&
        fork_height <= height;

    tx.metadata.existed = is_linked;
    tx.metadata.link = result.link();
    tx.metadata.candidate = result.candidate();
    tx.metadata.confirmed = is_confirmed;
    tx.metadata.cataloged = is_cataloged;
    tx.metadata.verified = !is_confirmed && height == forks;
}

void transaction_database::get_pool_metadata(const chain::transaction& tx,
    uint32_t forks) const
{
    static const auto unlinked = transaction::validation::unlinked;
    static const auto unconfirmed = transaction_result::unconfirmed;
    static const auto deconfirmed = transaction_result::deconfirmed;
    const auto result = get(tx.hash());

    // Default values presumed correct for indication of not found.
    if (!result)
        return;

    const auto height = result.height();
    const auto is_linked = result.link() != unlinked;
    const auto is_confirmed = result.position() != unconfirmed &&
        result.position() != deconfirmed;
    const auto is_cataloged =  result.position() != unconfirmed;

    tx.metadata.existed = is_linked;
    tx.metadata.link = result.link();
    tx.metadata.candidate = result.candidate();
    tx.metadata.confirmed = is_confirmed;
    tx.metadata.cataloged = is_cataloged;
    tx.metadata.verified = !is_confirmed && height == forks;
}

// Metadata should be defaulted by caller.
bool transaction_database::get_output(const output_point& point,
    size_t fork_height) const
{
    static const auto not_spent = output::validation::not_spent;
    static const auto unconfirmed = transaction_result::unconfirmed;
    static const auto deconfirmed = transaction_result::deconfirmed;
    auto& prevout = point.metadata;

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
    const auto prevout_height = result.height();
    if (prevout_height == 0)
        return false;

    // Find the output at the specified index for the found tx.
    prevout.cache = result.output(point.index());
    if (!prevout.cache.is_valid())
        return false;

    const auto position = result.position();
    const auto first_position = position == 0;
    const auto has_position = position != unconfirmed &&
        position != deconfirmed;
    const auto spender_height = prevout.cache.metadata.confirmed_spent_height;

    // Populate output metadata relative to the fork point.
    prevout.height = prevout_height;
    prevout.median_time_past = result.median_time_past();
    prevout.coinbase = first_position;
    prevout.candidate = result.candidate();
    prevout.confirmed = has_position && prevout_height <= fork_height;
    prevout.candidate_spent = prevout.cache.metadata.candidate_spent;
    prevout.confirmed_spent = spender_height != not_spent &&
        spender_height <= fork_height;

    // May have a candidate or confirmed spend of a confirmed, but must not
    // have a confirmed spend of a candidate.
    BITCOIN_ASSERT(!(prevout.confirmed_spent && prevout.candidate));

    // Return is redundant with prevout.cache validity.
    return true;
}

// Store.
// ----------------------------------------------------------------------------

// Store new unconfirmed tx and set tx link metadata in any case.
bool transaction_database::store(const chain::transaction& tx, uint32_t forks)
{
    // Cache the unspent outputs of the unconfirmed transaction.
    cache_.add(tx, forks, no_time, false);

    return storize(tx, forks, no_time, transaction_result::unconfirmed);
}

// Store each tx and set tx link metadata for all.
bool transaction_database::store(const transaction::list& transactions)
{
    for (const auto& tx: transactions)
        if (!storize(tx, rule_fork::unverified, no_time,
            transaction_result::unconfirmed))
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
        const auto element = hash_table_.find(tx.hash());

        if (element)
            tx.metadata.link = element.link();
    }

    // This allows payment indexer to bypass indexing despite link existence.
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
    // Spend or unspend the candidate tx's previous outputs.
    for (const auto inpoint: get(link))
        if (!candidate_spend(inpoint, positive))
            return false;

    return candidize(link, positive);
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
bool transaction_database::candidize(link_type link, bool positive)
{
    const auto writer = [&](byte_serializer& serial)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(metadata_mutex_);
        serial.skip(height_size + position_size);
        serial.write_byte(positive ? transaction_result::candidate_true :
            transaction_result::candidate_false);
        ///////////////////////////////////////////////////////////////////////
    };

    const auto element = hash_table_.get(link);
    element.write(writer);
    return true;
}

// Confirm/Unconfirm.
// ----------------------------------------------------------------------------

// This does not add to the output cache since it would require reading the tx.
// This also does not confirm unconfirmed cached outputs (should not exist).
// This also does not remove cached items on unconfirmation (should not call).
bool transaction_database::confirm(file_offset link, size_t height,
    uint32_t median_time_past, size_t position)
{
    // Avoid population of prevouts for coinbase (confirmation optimization).
    if (position != 0)
    {
        const auto result = get(link);

        if (!result)
            return false;

        // Spend or unspend the tx's previous outputs.
        for (const auto inpoint: result)
            if (!confirmed_spend(inpoint, height))
                return false;
    }

    // Promote or demote the tx.
    return confirmize(link, height, median_time_past, position);
}

bool transaction_database::confirm(const block& block, size_t height,
    uint32_t median_time_past)
{
    uint32_t position = 0;
    for (const auto& tx: block.transactions())
    {
        if (!confirm(tx.metadata.link, height, median_time_past, position++))
            return false;

        // Candidates are not cached but this only affects branch length > 1.
        // Cache the unspent outputs of the confirmed transaction.
        cache_.add(tx, height, median_time_past, true);
    }

    return true;
}

// Should only be called for a confirmed block
bool transaction_database::unconfirm(const block& block)
{
    for (const auto& tx: block.transactions())
    {
        if (!confirm(tx.metadata.link, rule_fork::unverified, no_time,
            transaction_result::deconfirmed))
            return false;

        // Uncache the unspent outputs of the unconfirmed transaction.
        cache_.remove(tx.hash());
    }

    return true;
}

// private
bool transaction_database::confirmed_spend(const output_point& point,
    size_t spender_height)
{
    // This just simplifies calling by allowing coinbase to be included.
    if (point.is_null() || spender_height > max_uint32)
        return true;

    const auto element = hash_table_.find(point.hash());
    auto spend_height = static_cast<uint32_t>(spender_height);

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

    // Limit to confirmed prevouts at or below the spender height.
    if (position == transaction_result::unconfirmed || height > spend_height)
        return false;

    // The index is not in the transaction.
    if (point.index() >= outputs)
        return false;

    // Use not_spent as the spender_height for output.
    if (spend_height == rule_fork::unverified)
        spend_height = output::validation::not_spent;

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

        serial.write_4_bytes_little_endian(spend_height);
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

    const auto element = hash_table_.get(link);
    element.write(writer);
    return true;
}

} // namespace database
} // namespace libbitcoin
