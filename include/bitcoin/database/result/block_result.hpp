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

/// Partially-deferred read block result.
/// Values subject to change are not read-deferred.
/// Transaction values are either empty or permanent.
class BCD_API block_result
{
public:
    block_result(const record_manager& index_manager);

    // TODO: review.
    block_result(const record_manager& index_manager, memory_ptr record,
        hash_digest&& hash, uint32_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, bool confirmed);

    // TODO: review.
    block_result(const record_manager& index_manager, memory_ptr record,
        const hash_digest& hash, uint32_t height, uint32_t checksum,
        array_index tx_start, size_t tx_count, bool confirmed);

    /// True if the requested block exists.
    operator bool() const;

    /// Reset the record pointer so that no lock is held.
    void reset();

    // TODO: review.
    /// True if the block is presently in the strong chain.
    bool confirmed() const;

    // TODO: review.
    /// The height of the block in its chain.
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
    const bool confirmed_;
    const record_manager& index_manager_;
};

} // namespace database
} // namespace libbitcoin

#endif
