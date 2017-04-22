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

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr size_t version_size = sizeof(uint32_t);
static constexpr size_t previous_size = hash_size;
static constexpr size_t merkle_size = hash_size;
static constexpr size_t time_size = sizeof(uint32_t);
static constexpr size_t bits_size = sizeof(uint32_t);
static constexpr size_t nonce_size = sizeof(uint32_t);
static constexpr size_t height_size = sizeof(uint32_t);

static constexpr auto version_offset = 0u;
static constexpr auto time_offset = version_size + previous_size + merkle_size;
static constexpr auto bits_offset = time_offset + time_size;
static constexpr auto height_offset = bits_offset + bits_size + nonce_size;
static constexpr auto count_offset = height_offset + height_size;

block_result::block_result()
  : block_result(nullptr)
{
}

block_result::block_result(const memory_ptr slab)
  : slab_(slab), height_(0), hash_(null_hash)
{
}

block_result::block_result(const memory_ptr slab, hash_digest&& hash,
    uint32_t height)
  : slab_(slab), height_(height), hash_(std::move(hash))
{
}

block_result::block_result(const memory_ptr slab, const hash_digest& hash,
    uint32_t height)
  : slab_(slab), height_(height), hash_(hash)
{
}

block_result::operator bool() const
{
    return slab_ != nullptr;
}

void block_result::reset()
{
    slab_.reset();
}

const hash_digest& block_result::hash() const
{
    return hash_;
}

chain::header block_result::header() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab_));

    // READ THE HEADER
    chain::header header;
    header.from_data(deserial);

    // TODO: add hash param to deserialization to eliminate this move.
    return chain::header(std::move(header), hash_digest(hash_));
}

// TODO: block height is unguarded and will be inconsistent during write.
size_t block_result::height() const
{
    BITCOIN_ASSERT(slab_);
    return height_;
}

uint32_t block_result::bits() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + bits_offset);
}

uint32_t block_result::timestamp() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + time_offset);
}

uint32_t block_result::version() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + version_offset);
}

size_t block_result::transaction_count() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + count_offset);
    return deserial.read_size_little_endian();
}

hash_digest block_result::transaction_hash(size_t index) const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + count_offset);
    const auto tx_count = deserial.read_size_little_endian();

    BITCOIN_ASSERT(index < tx_count);
    deserial.skip(index * hash_size);
    return deserial.read_hash();
}

hash_list block_result::transaction_hashes() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + count_offset);
    const auto tx_count = deserial.read_size_little_endian();
    hash_list hashes;
    hashes.reserve(tx_count);

    for (size_t position = 0; position < tx_count; ++position)
        hashes.push_back(deserial.read_hash());

    return hashes;
}

} // namespace database
} // namespace libbitcoin
