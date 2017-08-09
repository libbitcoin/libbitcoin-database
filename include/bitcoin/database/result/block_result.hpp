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
#ifndef LIBBITCOIN_DATABASE_BLOCK_RESULT_HPP
#define LIBBITCOIN_DATABASE_BLOCK_RESULT_HPP

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

// Stored block headers are always valid (PoW) with height, states are:
enum block_state : uint8_t
{
    /// This is not a stored state.
    missing = 0,

    /// Stored headers are always valid, these refer to their blocks.
    /// Mutually-exclusive (invalid is not pooled, only pent may be empty).
    failed = 1 << 0,
    pent = 1 << 1,
    valid = 1 << 2,

    /// Mutually-exclusive (confirmed must be valid, confirmed can't be empty).
    pooled = 1 << 3,
    indexed = 1 << 4,
    confirmed = 1 << 5,

    validations = invalid | pent | valid,
    confirmations = pooled | indexed | confirmed
};

// validation states

// This is not the same as !valid (could be pent).
inline bool is_failed(uint8_t state)
{
    return (state & block_state::failed) != 0;
}

inline bool is_pent(uint8_t state)
{
    return (state & block_state::pent) != 0;
}

inline bool is_valid(uint8_t state)
{
    return (state & block_state::valid) != 0;
}

// confirmation states

inline bool is_pooled(uint8_t state)
{
    return (state & block_state::pooled) != 0;
}

inline bool is_indexed(uint8_t state)
{
    return (state & block_state::indexed) != 0;
}

inline bool is_confirmed(uint8_t state)
{
    return (state & block_state::confirmed) != 0;
}

/// Partially-deferred read block result.
/// Values subject to change are not read-deferred.
/// Transaction values are either empty or permanent.
class BCD_API block_result
{
public:
    block_result(const record_manager& index_manager);

    block_result(const record_manager& index_manager, memory_ptr record,
        hash_digest&& hash, uint32_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, uint8_t state);
    block_result(const record_manager& index_manager, memory_ptr record,
        const hash_digest& hash, uint32_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, uint8_t state);

    /// True if the requested block exists.
    operator bool() const;

    /// Reset the record pointer so that no lock is held.
    void reset();

    /// An error code if block state is invalid (otherwise error::success).
    code error() const;

    /// The state of the block (flags).
    uint8_t state() const;

    /// The height of the block (independent of chain).
    size_t height() const;

    /// The block header hash (from cache).
    const hash_digest& hash() const;

    /// The block header.
    chain::header header() const;

    /// The header.bits of this block.
    uint32_t bits() const;

    /// The header.timestamp of this block.
    uint32_t timestamp() const;

    /// The header.version of this block.
    uint32_t version() const;

    /// The full block p2p message checksum (presumed invalid if zero).
    uint32_t checksum() const;

    /// The number of transactions in this block (may be zero).
    size_t transaction_count() const;

    /// Get the set of transaction offsets into the tx table for the block.
    offset_list transaction_offsets() const;

private:
    memory_ptr record_;
    const hash_digest hash_;
    const uint32_t height_;
    const uint32_t checksum_;
    const array_index tx_start_;
    const size_t tx_count_;
    const uint8_t state_;
    const record_manager& index_manager_;
};

} // namespace database
} // namespace libbitcoin

#endif
