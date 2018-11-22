/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

accessor::accessor(system::upgrade_mutex& mutex)
  : mutex_(mutex), data_(nullptr)
{
    ///////////////////////////////////////////////////////////////////////////
    // Begin Critical Section
    mutex_.lock_upgrade();
}

uint8_t* accessor::buffer()
{
    return data_;
}

// Assign a buffer to this upgradable allocator.
void accessor::assign(uint8_t* data)
{
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    mutex_.unlock_upgrade_and_lock_shared();
    data_ = data;
}

void accessor::increment(size_t value)
{
    BITCOIN_ASSERT_MSG(data_ != nullptr, "Buffer not assigned.");
    BITCOIN_ASSERT((size_t)data_ <= bc::max_size_t - value);

    data_ += value;
}

accessor::~accessor()
{
    mutex_.unlock_shared();
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace database
} // namespace libbitcoin
