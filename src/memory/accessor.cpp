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
#include <bitcoin/database/memory/accessor.hpp>

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

accessor::accessor(shared_mutex& mutex)
  : mutex_(mutex)
{
    ///////////////////////////////////////////////////////////////////////////
    // Begin Critical Section
    mutex_.lock_upgrade();
}

accessor::accessor(shared_mutex& mutex, uint8_t*& data)
  : mutex_(mutex)
{
    BITCOIN_ASSERT_MSG(data != nullptr, "Invalid buffer assigment.");

    ///////////////////////////////////////////////////////////////////////////
    // Begin Critical Section
    mutex_.lock_shared();
    data_ = data;
}

uint8_t* accessor::buffer()
{
    return data_;
}

void accessor::increment(size_t value)
{
    BITCOIN_ASSERT_MSG(data != nullptr, "Buffer not assigned.");
    BITCOIN_ASSERT((size_t)data_ <= bc::max_size_t - value);

    data_ += value;
}

// Assign a buffer to this upgradable allocator.
void accessor::assign(uint8_t* data)
{
    BITCOIN_ASSERT_MSG(data != nullptr, "Invalid buffer assigment.");

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    mutex_.unlock_upgrade_and_lock_shared();
    data_ = data;
}

accessor::~accessor()
{
    mutex_.unlock_shared();
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace database
} // namespace libbitcoin
