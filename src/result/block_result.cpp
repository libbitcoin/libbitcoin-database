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
#include <bitcoin/database/result/block_result.hpp>

#include <cstdint>
#include <cstddef>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/state/block_state.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr size_t version_size = sizeof(uint32_t);
static constexpr size_t previous_size = hash_size;
static constexpr size_t merkle_size = hash_size;
static constexpr size_t time_size = sizeof(uint32_t);

static constexpr auto version_offset = 0u;
static constexpr auto time_offset = version_size + previous_size + merkle_size;
static constexpr auto bits_offset = time_offset + time_size;

block_result::block_result(const record_manager& index_manager)
  : record_(nullptr),
    hash_(null_hash),
    height_(0),
    checksum_(0),
    tx_start_(0),
    tx_count_(0),
    state_(block_state::missing),
    index_manager_(index_manager)
{
}

block_result::block_result(const record_manager& index_manager,
    memory_ptr record, hash_digest&& hash, uint32_t height,
    uint32_t checksum, array_index tx_start, size_t tx_count, uint8_t state)
  : record_(record),
    hash_(std::move(hash)),
    height_(height),
    checksum_(checksum),
    tx_start_(tx_start),
    tx_count_(tx_count),
    state_(state),
    index_manager_(index_manager)
{
}

block_result::block_result(const record_manager& index_manager,
    memory_ptr record, const hash_digest& hash, uint32_t height,
    uint32_t checksum, array_index tx_start, size_t tx_count, uint8_t state)
  : record_(record),
    hash_(hash),
    height_(height),
    checksum_(checksum),
    tx_start_(tx_start),
    tx_count_(tx_count),
    state_(state),
    index_manager_(index_manager)
{
}

block_result::operator bool() const
{
    return record_ != nullptr;
}

void block_result::reset()
{
    record_.reset();
}

code block_result::error() const
{
    // Checksum stores error code if the block is invalid.
    return is_failed(state_) ? static_cast<error::error_code_t>(checksum_) :
        error::success;
}

uint8_t block_result::state() const
{
    return state_;
}

size_t block_result::height() const
{
    return height_;
}

const hash_digest& block_result::hash() const
{
    return hash_;
}

chain::header block_result::header() const
{
    BITCOIN_ASSERT(record_);
    auto deserial = make_unsafe_deserializer(record_->buffer());
    return header::factory(deserial, hash_);
}

uint32_t block_result::bits() const
{
    BITCOIN_ASSERT(record_);
    const auto memory = record_->buffer();
    return from_little_endian_unsafe<uint32_t>(memory + bits_offset);
}

uint32_t block_result::timestamp() const
{
    BITCOIN_ASSERT(record_);
    const auto memory = record_->buffer();
    return from_little_endian_unsafe<uint32_t>(memory + time_offset);
}

uint32_t block_result::version() const
{
    BITCOIN_ASSERT(record_);
    const auto memory = record_->buffer();
    return from_little_endian_unsafe<uint32_t>(memory + version_offset);
}

uint32_t block_result::checksum() const
{
    return checksum_;
}

size_t block_result::transaction_count() const
{
    return tx_count_;
}

offset_list block_result::transaction_offsets() const
{
    const auto end = tx_start_ + tx_count_;
    if (end > index_manager_.count())
        return{};

    const auto records = index_manager_.get(tx_start_);
    if (!records)
        return{};

    offset_list value;
    value.reserve(tx_count_);
    auto deserial = make_unsafe_deserializer(records->buffer());

    for (size_t index = 0; index < tx_count_; ++index)
        value.push_back(deserial.read_8_bytes_little_endian());

    return value;
}

} // namespace database
} // namespace libbitcoin
