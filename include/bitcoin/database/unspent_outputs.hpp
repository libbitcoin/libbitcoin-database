/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_OUTPUTS_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_OUTPUTS_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/unspent_transaction.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe.
/// A circular-by-age hash table of [point, output].
class BCD_API unspent_outputs
  : system::noncopyable
{
public:
    // Construct a cache with the specified transaction count limit.
    unspent_outputs(size_t capacity);

    /// The cache capacity is zero.
    bool disabled() const;

    /// The cache has no elements.
    size_t empty() const;

    /// The number of elements in the cache.
    size_t size() const;

    /// The cache performance as a ratio of hits to accesses.
    float hit_rate() const;

    /// Add outputs to cache, unconfirmed height is forks (purges matching tx).
    void add(const system::chain::transaction& tx, size_t height,
        uint32_t median_time_past, bool confirmed);

    /// Remove outputs from the cache (tx has been reorganized out).
    void remove(const system::hash_digest& tx_hash);

    /// Remove one output from the cache (has been confirmed spent).
    void remove(const system::chain::output_point& point);

    /// Populate output if cached/unspent relative to fork height.
    bool populate(const system::chain::output_point& point,
        size_t fork_height=max_size_t) const;

private:
    // A bidirection map is used for efficient output and position retrieval.
    // This produces the effect of a circular buffer tx hash table of outputs.
    typedef boost::bimaps::bimap<
        boost::bimaps::unordered_set_of<unspent_transaction>,
        boost::bimaps::set_of<uint32_t>> unspent_transactions;

    // These are thread safe.
    const size_t capacity_;
    mutable std::atomic<size_t> hits_;
    mutable std::atomic<size_t> queries_;

    // These are protected by mutex.
    uint32_t sequence_;
    unspent_transactions unspent_;
    mutable system::upgrade_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
