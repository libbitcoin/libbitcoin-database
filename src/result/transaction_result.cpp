/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/result/transaction_result.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr size_t value_size = sizeof(uint64_t);
static constexpr size_t height_size = sizeof(uint32_t);
static constexpr size_t version_size = sizeof(uint32_t);
static constexpr size_t sequence_size = sizeof(uint32_t);
static constexpr size_t position_size = sizeof(uint32_t);
static constexpr size_t outpoint_size = sizeof(hash_digest) + sizeof(uint32_t);

transaction_result::transaction_result(const memory_ptr slab)
  : slab_(slab)
{
}

transaction_result::operator bool() const
{
    return slab_ != nullptr;
}

size_t transaction_result::height() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory);
}

size_t transaction_result::position() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + height_size);
}

// If index is out of range this returns an invalid output (.value not_found).
chain::output transaction_result::output(uint32_t index) const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    const auto tx_start = memory + height_size + position_size;
    auto serial = make_deserializer_unsafe(tx_start);

    // Skip the transaction version.
    serial.skip_bytes(version_size);

    // Read the number of inputs (variable).
    const auto inputs = serial.read_variable_uint_little_endian();

    // Skip all inputs (it would be optimal to store outputs before inputs).
    for (uint64_t input = 0; input < inputs; ++input)
    {
        serial.skip_bytes(outpoint_size);
        const auto script_size = serial.read_variable_uint_little_endian();
        BITCOIN_ASSERT(script_size <= max_size_t);
        serial.skip_bytes(static_cast<size_t>(script_size) + sequence_size);
    }

    // Read the number of inputs (variable, but point-limited to max_uint32).
    const auto outputs = serial.read_variable_uint_little_endian();
    BITCOIN_ASSERT(outputs <= max_uint32);

    // The caller requested an output that does not exist in the transaction.
    if (index >= outputs)
        return{};

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < index; ++output)
    {
        serial.skip_bytes(value_size);
        const auto script_size = serial.read_variable_uint_little_endian();
        BITCOIN_ASSERT(script_size <= max_size_t);
        serial.skip_bytes(static_cast<size_t>(script_size));
    }

    chain::output output;
    output.from_data(serial);
    return output;
}

chain::transaction transaction_result::transaction() const
{
    BITCOIN_ASSERT(slab_);
    const auto memory = REMAP_ADDRESS(slab_);
    const auto tx_start = memory + height_size + position_size;
    auto deserial = make_deserializer_unsafe(tx_start);

    chain::transaction tx;
    tx.from_data(deserial);
    return tx;
}
} // namespace database
} // namespace libbitcoin
