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
#include "../test.hpp"
#include "storage.hpp"

#include <memory>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database.hpp>

namespace test {

// This is a trivial working storage interface implementation.
storage::storage() NOEXCEPT
  : mapped_(false)
{
}

storage::storage(data_chunk&& initial) NOEXCEPT
  : mapped_(false), buffer_(std::move(initial))
{
}

storage::storage(const data_chunk& initial) NOEXCEPT
  : mapped_(false), buffer_(initial)
{
}

storage::~storage() NOEXCEPT
{
    unmap();
}

bool storage::map() NOEXCEPT
{
    mutex_.lock_upgrade();

    if (mapped_)
    {
        mutex_.unlock_upgrade();
        return true;
    }

    mutex_.unlock_upgrade_and_lock();
    mapped_ = true;
    mutex_.unlock();
    return true;
}

bool storage::flush() const NOEXCEPT
{
    return true;
}

bool storage::unmap() NOEXCEPT
{
    mutex_.lock_upgrade();

    if (!mapped_)
    {
        mutex_.unlock_upgrade();
        return true;
    }

    mutex_.unlock_upgrade_and_lock();
    mapped_ = false;
    mutex_.unlock();
    return true;
}

bool storage::mapped() const NOEXCEPT
{
    std::shared_lock lock(mutex_);
    return mapped_;
}

size_t storage::capacity() const NOEXCEPT
{
    return logical();
}

size_t storage::logical() const NOEXCEPT
{
    std::shared_lock lock(mutex_);
    return buffer_.size();
}

memory_ptr storage::access() NOEXCEPT(false)
{
    const auto memory = std::make_shared<accessor>(mutex_);
    memory->assign(buffer_.data());
    return memory;
}

memory_ptr storage::resize(size_t size) NOEXCEPT(false)
{
    return reserve(size);
}

memory_ptr storage::reserve(size_t size) NOEXCEPT(false)
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
