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

// Because of BIP30 it is safe to use tx hashes as identifiers here.
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

void unspent_outputs::add(const transaction& transaction, size_t height,
    bool confirmed)
{
    if (disabled() || transaction.outputs().empty())
        return;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // It's been a long time since the last restart (~16 years).
    if (sequence_ == max_uint32)
        unspent_.clear();

    // Remove the oldest entry if the buffer is at capacity.
    if (unspent_.size() >= capacity_)
        unspent_.right.erase(unspent_.right.begin());

    unspent_.insert(
    {
        unspent_transaction{ transaction, height, confirmed },
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

bool unspent_outputs::get(output& out_output, size_t& out_height,
    bool& out_coinbase, const output_point& point, size_t fork_height,
    bool require_confirmed) const
{
    if (disabled())
        return false;

    ++queries_;
    const unspent_transaction key{ point };

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    // Find the unspent tx entry.
    const auto tx = unspent_.left.find(key);

    if (tx == unspent_.left.end() ||
        (require_confirmed && !tx->first.is_confirmed()))
        return false;

    // Find the output at the specified index for the found unspent tx.
    const auto outputs = tx->first.outputs();
    const auto output = outputs->find(point.index());

    if (output == outputs->end())
        return false;

    // Determine if the cached unspent tx is above specified fork_height.
    // Since the hash table does not allow duplicates there are no others.
    const auto& unspent = tx->first;
    const auto height = unspent.height();

    if (height > fork_height)
        return false;

    ++hits_;
    out_height = height;
    out_coinbase = unspent.is_coinbase();
    out_output = output->second;
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace database
} // namespace libbitcoin
