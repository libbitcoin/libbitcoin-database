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
#include <bitcoin/database/memory/memory_allocator.hpp>

#include <cstdint>
#include <boost/thread.hpp>

namespace libbitcoin {
namespace database {

using namespace boost;

memory_allocator::memory_allocator(uint8_t* data, shared_mutex& mutex)
  : data_(data),
    mutex_(mutex),
    upgradeable_lock_(mutex_)
{
    ///////////////////////////////////////////////////////////////////////
    // Begin Critical Section
    ///////////////////////////////////////////////////////////////////////
}

uint8_t* memory_allocator::buffer()
{
    return data_;
}

void memory_allocator::increment(size_t value)
{
    BITCOIN_ASSERT(data_ <= bc::max_size_t - value);
    data_ += value;
}

// protected/friend
upgrade_lock<shared_mutex>& memory_allocator::get_upgradeable()
{
    return upgradeable_lock_;
}

// protected/friend
void memory_allocator::set_data(uint8_t* value)
{
    data_ = value;
}

memory_allocator::~memory_allocator()
{
    ///////////////////////////////////////////////////////////////////////
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////
}

} // namespace database
} // namespace libbitcoin
