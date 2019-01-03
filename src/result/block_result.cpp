/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/result/block_result.hpp>

#include <cstdint>
#include <cstddef>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/block_state.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/result/transaction_iterator.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::system;
using namespace bc::system::chain;

static const auto header_size = header::satoshi_fixed_size();
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto state_size = sizeof(uint8_t);
static constexpr auto checksum_size = sizeof(uint32_t);
static constexpr auto tx_start_size = sizeof(uint32_t);
static constexpr auto tx_count_size = sizeof(uint16_t);

static const auto height_offset = header_size + median_time_past_size;
static const auto state_offset = height_offset + height_size;
static const auto checksum_offset = state_offset + state_size;
static const auto transactions_offset = checksum_offset + checksum_size;

// Placeholder for unimplemented checksum caching.
static constexpr auto no_checksum = 0u;

block_result::block_result(const const_element_type& element,
    shared_mutex& metadata_mutex, const manager& index_manager)
  : height_(0),
    median_time_past_(0),
    state_(block_state::missing),
    checksum_(no_checksum),
    tx_start_(0),
    tx_count_(0),
    element_(element),
    index_manager_(index_manager),
    metadata_mutex_(metadata_mutex)
{
    if (!element_)
        return;

    const auto reader = [&](byte_deserializer& deserial)
    {
        // These are never updated.
        header_.from_data(deserial, element_.key(), false);
        median_time_past_ = deserial.read_4_bytes_little_endian();
        height_ = deserial.read_4_bytes_little_endian();

        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(metadata_mutex_);
        state_ = deserial.read_byte();
        checksum_ = deserial.read_4_bytes_little_endian();
        tx_start_ = deserial.read_4_bytes_little_endian();
        tx_count_ = deserial.read_2_bytes_little_endian();
        ///////////////////////////////////////////////////////////////////////
    };

    // Reads not deferred for updatable values as consistency is required.
    element_.read(reader);
}

block_result::operator bool() const
{
    return element_;
}

code block_result::error() const
{
    // Checksum stores error code if the block is invalid.
    return is_failed(state_) ? static_cast<error::error_code_t>(checksum_) :
        error::success;
}

array_index block_result::link() const
{
    return element_.link();
}

// This is read each time it is invoked, so caller should cache.
hash_digest block_result::hash() const
{
    return element_ ? element_.key() : null_hash;
}

void block_result::set_metadata(const chain::header& header) const
{
    if ((header.metadata.exists = element_))
    {
        const auto state = this->state();
        header.metadata.error = error();
        header.metadata.candidate = is_candidate(state);
        header.metadata.confirmed = is_confirmed(state);
        header.metadata.validated = is_valid(state) || is_failed(state);
        header.metadata.populated = transaction_count() != 0;
        header.metadata.median_time_past = median_time_past();
    }
}

// This is read each time it is invoked, so caller should cache.
chain::header block_result::header(bool metadata) const
{
    if (!element_)
        return {};

    chain::header header;
    const auto reader = [&](byte_deserializer& deserial)
    {
        header.from_data(deserial, element_.key(), false);
    };

    element_.read(reader);

    if (metadata)
        set_metadata(header);

    return header;
}

uint32_t block_result::bits() const
{
    return header_.bits();
}

uint32_t block_result::timestamp() const
{
    return header_.timestamp();
}

uint32_t block_result::version() const
{
    return header_.version();
}

uint32_t block_result::median_time_past() const
{
    return median_time_past_;
}

size_t block_result::height() const
{
    return height_;
}

uint8_t block_result::state() const
{
    return state_;
}

uint32_t block_result::checksum() const
{
    return checksum_;
}

size_t block_result::transaction_count() const
{
    return tx_count_;
}

transaction_iterator block_result::begin() const
{
    return { index_manager_, tx_start_, tx_count_ };
}

transaction_iterator block_result::end() const
{
    return { index_manager_, tx_start_, 0 };
}

} // namespace database
} // namespace libbitcoin
