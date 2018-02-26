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
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/result/transaction_iterator.hpp>

namespace libbitcoin {
namespace database {

/// Partially-deferred read block result.
/// Values subject to change are not read-deferred.
/// Transaction values are either empty (0 count) or permanent.
class BCD_API block_result
{
public:
    typedef hash_digest key_type;
    typedef array_index link_type;
    typedef record_manager<link_type> manager;
    typedef list_element<const manager, link_type, key_type>
        const_element_type;

    block_result(const const_element_type& element,
        shared_mutex& metadata_mutex, const manager& index_manager);

    /// True if the requested block exists.
    operator bool() const;

    /// An error code if block state is invalid (otherwise error::success).
    code error() const;

    /// The link for the block slab.
    array_index link() const;

    /// The block header hash (from cache).
    hash_digest hash() const;

    /// The block header.
    const chain::header& header() const;

    /// The header.bits of this block.
    uint32_t bits() const;

    /// The header.timestamp of this block.
    uint32_t timestamp() const;

    /// The header.version of this block.
    uint32_t version() const;

    /// The median time past of the block which includes the transaction.
    uint32_t median_time_past() const;

    /// The height of the block (independent of chain).
    size_t height() const;

    /// The state of the block (flags).
    uint8_t state() const;

    /// The full block p2p message checksum (presumed invalid if zero).
    uint32_t checksum() const;

    /// The number of transactions in this block (may be zero).
    size_t transaction_count() const;

    /// Iterate over the transaction link set.
    transaction_iterator begin() const;
    transaction_iterator end() const;

private:
    chain::header header_;
    uint32_t median_time_past_;
    uint32_t height_;
    uint8_t state_;
    uint32_t checksum_;
    array_index tx_start_;
    size_t tx_count_;

    // These classes are thread safe.
    const const_element_type element_;
    const manager& index_manager_;

    // Metadata values are kept consistent by mutex.
    shared_mutex& metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
