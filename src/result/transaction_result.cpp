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
#include <bitcoin/database/result/transaction_result.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint16_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto metadata_size = height_size + position_size +
    median_time_past_size;

transaction_result::transaction_result()
  : transaction_result(nullptr)
{
}

transaction_result::transaction_result(memory_ptr slab)
  : slab_(nullptr),
    height_(0),
    median_time_past_(0),
    position_(0),
    hash_(null_hash)
{
}

transaction_result::transaction_result(memory_ptr slab, hash_digest&& hash,
    uint32_t height, uint32_t median_time_past, uint16_t position)
  : slab_(slab),
    height_(height),
    median_time_past_(median_time_past),
    position_(position),
    hash_(std::move(hash))
{
}

transaction_result::transaction_result(memory_ptr slab,
    const hash_digest& hash, uint32_t height, uint32_t median_time_past,
    uint16_t position)
  : slab_(slab),
    height_(height),
    median_time_past_(median_time_past),
    position_(position),
    hash_(hash)
{
}

transaction_result::operator bool() const
{
    return slab_ != nullptr;
}

void transaction_result::reset()
{
    slab_.reset();
}

bool transaction_result::confirmed() const
{
    return position_ != transaction_database::unconfirmed;
}

size_t transaction_result::position() const
{
    return position_;
}

size_t transaction_result::height() const
{
    return height_;
}

const hash_digest& transaction_result::hash() const
{
    return hash_;
}

// Median time past is unguarded and will be inconsistent during write.
uint32_t transaction_result::median_time_past() const
{
    BITCOIN_ASSERT(slab_);
    return median_time_past_;
}

// Spentness is unguarded and will be inconsistent during write.
bool transaction_result::is_spent(size_t fork_height) const
{
    // Cannot be spent if unconfirmed.
    if (!confirmed())
        return false;

    BITCOIN_ASSERT(slab_);
    const auto tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    const auto outputs = deserial.read_size_little_endian();
    BITCOIN_ASSERT(deserial);

    // Search all outputs for an unspent indication.
    for (uint32_t output = 0; output < outputs; ++output)
    {
        const auto spender_height = deserial.read_4_bytes_little_endian();
        BITCOIN_ASSERT(deserial);

        // A spend from above the fork height is not an actual spend.
        if (spender_height == output::validation::not_spent ||
            spender_height > fork_height)
            return false;

        deserial.skip(value_size);
        deserial.skip(deserial.read_size_little_endian());
        BITCOIN_ASSERT(deserial);
    }

    return true;
}

// Spentness is unguarded and will be inconsistent during write.
// If index is out of range returns default/invalid output (.value not_found).
chain::output transaction_result::output(uint32_t index) const
{
    BITCOIN_ASSERT(slab_);
    const auto tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    const auto outputs = deserial.read_size_little_endian();
    BITCOIN_ASSERT(deserial);

    if (index >= outputs)
        return{};

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < index; ++output)
    {
        deserial.skip(height_size + value_size);
        deserial.skip(deserial.read_size_little_endian());
        BITCOIN_ASSERT(deserial);
    }

    // Read and return the target output (including spender height).
    return chain::output::factory(deserial, false);
}

// Spentness is unguarded and will be inconsistent during write.
chain::transaction transaction_result::transaction() const
{
    BITCOIN_ASSERT(slab_);
    const auto tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    return transaction::factory(deserial, hash_);
}

} // namespace database
} // namespace libbitcoin
