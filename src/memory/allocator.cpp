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
#include <bitcoin/database/memory/allocator.hpp>

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

#ifdef REMAP_SAFETY

allocator::allocator(shared_mutex& mutex)
  : mutex_(mutex),
    data_(nullptr)
{
    // Begin Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();
}

// Get share-protected data pointer.
uint8_t* allocator::buffer()
{
    BITCOIN_ASSERT_MSG(data_ != nullptr, "Downgrade must be called.");
    return data_;
}

// Add an unsafe offset to share-protected pointer (convenience method).
void allocator::increment(size_t value)
{
    BITCOIN_ASSERT((size_t)data_ <= bc::max_size_t - value);
    data_ += value;
}

// protected/friend
void allocator::assign(uint8_t* data)
{
    // The caller has the option to upgrade/downgrade but must have left the
    // the mutex in the original (upgradeable) state or this will fail.
    mutex_.unlock_upgrade_and_lock_shared();

    BITCOIN_ASSERT_MSG(data != nullptr, "Invalid pointer value.");
    data_ = data;
}

allocator::~allocator()
{
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.unlock_shared();
}

#endif // REMAP_SAFETY

} // namespace database
} // namespace libbitcoin
