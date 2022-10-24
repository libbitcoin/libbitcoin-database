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
#include "test.hpp"
#include "storage.hpp"

namespace test {

// locks may throw.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// This is a trivial working storage interface implementation.
storage::storage() NOEXCEPT
  : local_{}, buffer_{ local_ }
{
}

storage::storage(data_chunk& reference) NOEXCEPT
  : buffer_(reference)
{
}

size_t storage::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.capacity();
}

size_t storage::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.size();
}

bool storage::resize(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    if (size > buffer_.capacity())
        return false;

    buffer_.resize(size);
    return true;
}

size_t storage::allocate(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    std::unique_lock map_lock(map_mutex_);
    const auto link = buffer_.size();
    buffer_.resize(buffer_.size() + chunk);
    return link;
}

memory_ptr storage::get(size_t offset) NOEXCEPT
{
    const auto memory = std::make_shared<accessor<std::shared_mutex>>(map_mutex_);
    memory->assign(buffer_.data());
    memory->increment(offset);
    return memory;
}

BC_POP_WARNING()

} // namespace test
