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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_RESULT_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_RESULT_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

// Stored txs are verified or protected by valid header PoW, states are:
// TODO: compress into position using flag for indexed and sentinal for pool.
enum class transaction_state : uint8_t
{
    /// Interface only (not stored).
    missing = 0,

    /// Confirmable if forks match, height is forks, position unused.
    pooled = 1,

    /// Confirmed in header index, height is forks, position unused.
    indexed = 2,

    /// Confirmed in block index, height and position are block values.
    confirmed = 3
};

/// Deferred read transaction result.
class BCD_API transaction_result
{
public:
    /// This is unconfirmed tx height (forks) sentinel.
    static const uint32_t unverified;

    /// This is unconfirmed tx position sentinel.
    static const uint16_t unconfirmed;

    transaction_result();
    transaction_result(memory_ptr slab);
    transaction_result(memory_ptr slab, hash_digest&& hash, uint32_t height,
        uint32_t median_time_past, uint16_t position, transaction_state state,
        file_offset offset);
    transaction_result(memory_ptr slab, const hash_digest& hash,
        uint32_t height, uint32_t median_time_past, uint16_t position,
        transaction_state state, file_offset offset);

    /// True if this transaction result is valid (found).
    operator bool() const;

    /// Reset the slab pointer so that no lock is held.
    void reset();

    /// An error code if block state is invalid.
    code error() const;

    /// The state of the transaction.
    transaction_state state() const;

    /// The ordinal position of the tx in a block, or unconfirmed.
    size_t position() const;

    /// The height of the block of the tx, or forks if unconfirmed.
    size_t height() const;

    /// The transaction hash (from cache).
    const hash_digest& hash() const;

    /// The file offset for the transaction slab.
    const file_offset offset() const;

    /// The median time past of the block which includes the transaction.
    uint32_t median_time_past() const;

    /// All tx outputs confirmed spent, ignore indexing if max fork point.
    bool is_spent(size_t fork_height=max_size_t) const;

    /// The output at the specified index within this transaction.
    chain::output output(uint32_t index) const;

    /// The transaction, optionally including witness.
    chain::transaction transaction(bool witness=true) const;

private:
    memory_ptr slab_;
    const uint32_t height_;
    const uint32_t median_time_past_;
    const uint16_t position_;
    const hash_digest hash_;
    const file_offset offset_;
    const transaction_state state_;
};

} // namespace database
} // namespace libbitcoin

#endif
