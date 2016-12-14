/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/unspent_outputs.hpp>

#include <cstddef>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

unspent_outputs::unspent_outputs(size_t capacity)
  : capacity_(capacity), hits_(1), queries_(1), sequence_(0)
{
}

size_t unspent_outputs::empty() const
{
    return unspent_.empty();
}

size_t unspent_outputs::size() const
{
    return unspent_.size();
}

float unspent_outputs::hit_rate() const
{
    // These values could overflow or divide by zero, but that's okay.
    return hits_ * 1.0f / queries_;
}

void unspent_outputs::add(const transaction& transaction, size_t height)
{
    if (capacity_ == 0 || transaction.outputs().empty())
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
        unspent_transaction{ transaction, height },
        ++sequence_
    });
    ///////////////////////////////////////////////////////////////////////////
}

void unspent_outputs::remove(const hash_digest& tx_hash)
{
    if (capacity_ == 0)
        return;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    const unspent_transaction key{ tx_hash };
    const auto tx = unspent_.left.find(key);

    if (tx != unspent_.left.end())
        unspent_.left.erase(tx);
    ///////////////////////////////////////////////////////////////////////////
}

void unspent_outputs::remove(const output_point& point)
{
    if (capacity_ == 0)
        return;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    const unspent_transaction key{ point };
    auto tx = unspent_.left.find(key);

    if (tx == unspent_.left.end())
        return;

    // Erase the output if found at the specified index for the found tx.
    const auto outputs = tx->first.outputs();
    outputs->erase(point.index());

    if (outputs->empty())
        unspent_.left.erase(tx);
    ///////////////////////////////////////////////////////////////////////////
}

bool unspent_outputs::get(output& out_output, size_t& out_height,
    bool& out_coinbase, const output_point& point, size_t fork_height) const
{
    if (capacity_ == 0)
        return false;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    ++queries_;
    const unspent_transaction key{ point };
    const auto tx = unspent_.left.find(key);

    if (tx == unspent_.left.end())
        return false;

    // Find the output at the specified index for the found tx.
    const auto outputs = tx->first.outputs();
    const auto output = outputs->find(point.index());

    if (output == outputs->end())
        return false;

    const auto& unspent = tx->first;
    const auto height = tx->first.height();

    // The cache entry is newer than specified.
    // Since the hash table does not allow duplicates there are no others.
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
