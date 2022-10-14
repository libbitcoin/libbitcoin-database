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
#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database.hpp>

namespace test {

// locks may throw.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// This is a trivial working storage interface implementation.
storage::storage() NOEXCEPT
  : mapped_(false), closed_(false), buffer_{}
{
}
    
storage::storage(data_chunk&& initial) NOEXCEPT
  : mapped_(false), closed_(false), buffer_(std::move(initial))
{
}

storage::storage(const data_chunk& initial) NOEXCEPT
  : mapped_(false), closed_(false), buffer_(initial)
{
}

storage::~storage() NOEXCEPT
{
}

bool storage::close() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    std::unique_lock map_lock(map_mutex_);
    closed_ = true;
    buffer_.clear();
    return true;
}

bool storage::load_map() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    std::unique_lock map_lock(map_mutex_);
    const auto mapped = mapped_;
    mapped_ = true;
    return !mapped;
}

bool storage::flush_map() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    std::unique_lock map_lock(map_mutex_);
    return mapped_;
}

bool storage::unload_map() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    std::unique_lock map_lock(map_mutex_);
    const auto mapped = mapped_;
    mapped_ = false;
    return mapped;
}

// Physical grows at same rate as logical (not time optimized).
// Allocation is never reduced in case of downsize (not space optimized).
memory_ptr storage::reserve(size_t size) THROWS
{
    std::unique_lock field_lock(field_mutex_);

    if (size != buffer_.size())
    {
        std::unique_lock map_lock(map_mutex_);
        buffer_.resize(size);
    }

    // No thread can intervene here because of the field_mutex_ lock.
    return get();
}

memory_ptr storage::resize(size_t size) THROWS
{
    return reserve(size);
}

memory_ptr storage::get() THROWS
{
    const auto memory = std::make_shared<accessor>(map_mutex_);
    memory->assign(buffer_.data());
    return memory;
}

bool storage::is_mapped() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return mapped_;
}

bool storage::is_closed() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return closed_;
}

size_t storage::logical() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.size();
}

size_t storage::capacity() const NOEXCEPT
{
    return logical();
}

size_t storage::size() const NOEXCEPT
{
    return logical();
}

BC_POP_WARNING()

} // namespace test
