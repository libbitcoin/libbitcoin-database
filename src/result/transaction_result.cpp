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
using namespace bc::machine;

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint16_t);
static constexpr auto state_size = sizeof(uint8_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);

static constexpr auto index_spend_size = sizeof(uint8_t);
////static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto value_size = sizeof(uint64_t);

static constexpr auto spend_size = index_spend_size + height_size + value_size;
static constexpr auto metadata_size = height_size + position_size +
    state_size + median_time_past_size;

const uint16_t transaction_result::unconfirmed = max_uint16;
const uint32_t transaction_result::unverified = rule_fork::unverified;

transaction_result::transaction_result(const const_element_type& element,
    shared_mutex& metadata_mutex)
  : height_(0),
    position_(unconfirmed),
    state_(transaction_state::missing),
    median_time_past_(0),
    element_(element),
    metadata_mutex_(metadata_mutex)
{
    if (!element_)
        return;

    // There is only one atomic set here.
    const auto reader = [&](byte_deserializer& deserial)
    {
        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        height_ = deserial.read_4_bytes_little_endian();
        position_ = deserial.read_2_bytes_little_endian();
        state_ = static_cast<transaction_state>(deserial.read_byte());
        median_time_past_ = deserial.read_4_bytes_little_endian();
        ///////////////////////////////////////////////////////////////////////
    };

    // Metadata reads not deferred for updatable values as atomicity required.
    element.read(reader);
}

transaction_result::operator bool() const
{
    return element_;
}

code transaction_result::error() const
{
    // Height could store error code, but we don't currently use invalid state.
    return error::success;
}

file_offset transaction_result::link() const
{
    return element_.link();
}

hash_digest transaction_result::hash() const
{
    // This is read each time it is invoked, so caller should cache.
    return element_ ? null_hash : element_.key();
}

size_t transaction_result::height() const
{
    // Height is overloaded (holds forks) unless confirmed.
    return height_;
}

size_t transaction_result::position() const
{
    // Position is unconfirmed unless if block-associated.
    return position_;
}

transaction_state transaction_result::state() const
{
    return state_;
}

uint32_t transaction_result::median_time_past() const
{
    return median_time_past_;
}

// Set fork_height to max_size_t for tx pool validation.
bool transaction_result::is_spent(size_t fork_height) const
{
    const auto relevant = height_ <= fork_height;
    const auto for_pool = fork_height == max_size_t;

    const auto confirmed =
        (state_ == transaction_state::indexed && !for_pool) ||
        (state_ == transaction_state::confirmed && relevant);

    // Cannot be spent unless confirmed.
    if (!confirmed)
        return false;

    BITCOIN_ASSERT(element_);
    auto spent = true;

    // Spentness is unguarded and will be inconsistent during write.
    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(metadata_size);
        const auto outputs = deserial.read_size_little_endian();

        // Search all outputs for an unspent indication.
        for (uint32_t out = 0; spent && out < outputs; ++out)
        {
            // TODO: This reads full output, which is simple but not optimial.
            const auto output = output::factory(deserial, false);
            spent = output.validation.spent(fork_height);
        }
    };

    element_.read(reader);
    return spent;
}

// If index is out of range returns default/invalid output (.value not_found).
chain::output transaction_result::output(uint32_t index) const
{
    BITCOIN_ASSERT(element_);
    chain::output output;

    // Spentness is unguarded and will be inconsistent during write.
    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(metadata_size);
        const auto outputs = deserial.read_size_little_endian();

        if (index >= outputs)
            return;

        // Skip outputs until the target output.
        for (uint32_t out = 0; out < index; ++out)
        {
            deserial.skip(spend_size);
            deserial.skip(deserial.read_size_little_endian());
        }

        // Read the target output.
        output.from_data(deserial, false);
    };

    // Read and return the target output (including spender height).
    element_.read(reader);
    return output;
}

// Spentness is unguarded and will be inconsistent during write.
chain::transaction transaction_result::transaction(bool witness) const
{
    BITCOIN_ASSERT(element_);
    chain::transaction tx;
    auto key = hash();

    const auto reader = [&](byte_deserializer& deserial)
    {
        deserial.skip(metadata_size);
        tx.from_data(deserial, std::move(key), false, witness);
    };

    element_.read(reader);
    return tx;
}

} // namespace database
} // namespace libbitcoin
