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
#include "test_map.hpp"

#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;

namespace test {

// This is a trivial working memory_map interface implementation.
test_map::test_map()
  : test_map(data_chunk{})
{
}

test_map::test_map(const data_chunk& initial)
  : closed_(true), buffer_(initial)
{
}

test_map::~test_map()
{
    close();
}

bool test_map::open()
{
    mutex_.lock_upgrade();

    if (closed_)
    {
        mutex_.unlock_upgrade();
        return true;
    }

    mutex_.unlock_upgrade_and_lock();
    closed_ = false;
    mutex_.unlock();
    return true;
}

bool test_map::flush() const
{
    return true;
}

bool test_map::close()
{
    mutex_.lock_upgrade();

    if (closed_)
    {
        mutex_.unlock_upgrade();
        return true;
    }

    mutex_.unlock_upgrade_and_lock();
    closed_ = true;
    mutex_.unlock();
    return true;
}

bool test_map::closed() const
{
    shared_lock lock(mutex_);
    return closed_;
}

size_t test_map::size() const
{
    shared_lock lock(mutex_);
    return buffer_.size();
}

memory_ptr test_map::access()
{
    return std::make_shared<accessor>(mutex_, buffer_.data());
}

memory_ptr test_map::resize(size_t size)
{
    return reserve(size);
}

memory_ptr test_map::reserve(size_t size)
{
    const auto memory = std::make_shared<accessor>(mutex_);

    // Physical grows at same rate as logical (not time optimized).
    // Allocation is never reduced in case of downsize (not space optimized).
    if (size != buffer_.size())
    {
        mutex_.unlock_upgrade_and_lock();
        buffer_.resize(size);
        mutex_.unlock_and_lock_upgrade();
    }

    memory->assign(buffer_.data());
    return memory;
}

} // namespace test
