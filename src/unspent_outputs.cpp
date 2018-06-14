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
#include <bitcoin/database/unspent_outputs.hpp>

#include <cstddef>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

// This does not differentiate indexed-block transactions. These are treated as
// unconfirmed, so this optimizes only for a top height fork point and tx pool.
unspent_outputs::unspent_outputs(size_t capacity)
  : capacity_(capacity), hits_(1), queries_(1), sequence_(0)
{
}

bool unspent_outputs::disabled() const
{
    return capacity_ == 0;
}

size_t unspent_outputs::empty() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    return unspent_.empty();
    ///////////////////////////////////////////////////////////////////////////
}

size_t unspent_outputs::size() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    return unspent_.size();
    ///////////////////////////////////////////////////////////////////////////
}

float unspent_outputs::hit_rate() const
{
    // These values could overflow or divide by zero, but that's okay.
    return hits_ * 1.0f / queries_;
}

void unspent_outputs::add(const transaction& tx, size_t height,
    uint32_t median_time_past, bool confirmed)
{
    if (disabled() || tx.outputs().empty())
        return;

    if (tx.is_coinbase())
    {
        LOG_DEBUG(LOG_DATABASE)
            << "Output cache hit rate: " << hit_rate() << ", size: " << size();
    }

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // It's been a long time since the last restart (~16 years).
    if (sequence_ == max_uint32)
        unspent_.clear();

    // Remove the oldest entry if the buffer is at capacity.
    if (unspent_.size() >= capacity_)
        unspent_.right.erase(unspent_.right.begin());

    // TODO: promote the unconfirmed tx cache instead of replacing it.
    // A confirmed tx may replace the same unconfirmed tx here.
    unspent_.insert(
    {
        unspent_transaction{ tx, height, median_time_past, confirmed },
        ++sequence_
    });
    ///////////////////////////////////////////////////////////////////////////
}

// This is confirmation-independent, since the conflict is extrememly rare and
// the difference is simply an optimization. This avoids dual key indexing.
void unspent_outputs::remove(const hash_digest& tx_hash)
{
    if (disabled())
        return;

    const unspent_transaction key{ tx_hash };

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    // Find the unspent tx entry.
    const auto tx = unspent_.left.find(key);

    if (tx == unspent_.left.end())
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return;
    }

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    unspent_.left.erase(tx);

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

// The output is confirmed spent, so remove it from unspent outputs.
void unspent_outputs::remove(const output_point& point)
{
    if (disabled())
        return;

    const unspent_transaction key{ point };

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    // Find the unspent tx entry that may contain the output.
    auto tx = unspent_.left.find(key);

    if (tx == unspent_.left.end())
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return;
    }

    const auto outputs = tx->first.outputs();
    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // Erase the output if found at the specified index for the found tx.
    outputs->erase(point.index());

    // Erase the unspent transaction if it is now fully spent.
    if (outputs->empty())
        unspent_.left.erase(tx);

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

// All responses are unspent, metadata should be defaulted by caller.
bool unspent_outputs::populate(const output_point& point,
    size_t fork_height) const
{
    if (disabled())
        return false;

    ++queries_;
    auto& prevout = point.metadata;
    const unspent_transaction key{ point };

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    // Find the unspent tx entry.
    const auto tx = unspent_.left.find(key);
    if (tx == unspent_.left.end())
        return false;

    // Find the output at the specified index for the found unspent tx.
    const auto& transaction = tx->first;
    const auto outputs = transaction.outputs();
    const auto output = outputs->find(point.index());
    if (output == outputs->end())
        return false;

    ++hits_;
    const auto height = transaction.height();

    // Populate the output metadata.
    prevout.spent = false;
    prevout.candidate = false;
    prevout.confirmed = transaction.is_confirmed() && height <= fork_height;
    prevout.coinbase = transaction.is_coinbase();
    prevout.height = height;
    prevout.median_time_past = transaction.median_time_past();
    prevout.cache = output->second;

    return true;
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace database
} // namespace libbitcoin
