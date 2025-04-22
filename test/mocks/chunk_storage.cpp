/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include "chunk_storage.hpp"
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <shared_mutex>

namespace test {

// locks may throw.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// This is a trivial working chunk_storage interface implementation.
chunk_storage::chunk_storage() NOEXCEPT
  : buffer_{ local_ }, path_{ "test" }
{
}

chunk_storage::chunk_storage(system::data_chunk& reference) NOEXCEPT
  : buffer_{ reference }, path_{ "test" }
{
}

chunk_storage::chunk_storage(const std::filesystem::path& filename,
    size_t, size_t) NOEXCEPT
  : buffer_{ local_ }, path_{ filename }
{
}

system::data_chunk& chunk_storage::buffer() NOEXCEPT
{
    return buffer_;
}

code chunk_storage::open() NOEXCEPT
{
    return error::success;
}

code chunk_storage::close() NOEXCEPT
{
    return error::success;
}

code chunk_storage::load() NOEXCEPT
{
    return error::success;
}

code chunk_storage::reload() NOEXCEPT
{
    return error::success;
}

code chunk_storage::flush() NOEXCEPT
{
    return error::success;
}

code chunk_storage::unload() NOEXCEPT
{
    return error::success;
}

const std::filesystem::path& chunk_storage::file() const NOEXCEPT
{
    return path_;
}

size_t chunk_storage::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.capacity();
}

size_t chunk_storage::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.size();
}

bool chunk_storage::truncate(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    if (size > buffer_.size())
        return false;

    buffer_.resize(size);
    return true;
}

bool chunk_storage::expand(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    if (size > buffer_.size())
        buffer_.resize(size);

    return true;
}

bool chunk_storage::reserve(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    buffer_.reserve(size);
    return true;
}

size_t chunk_storage::allocate(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    if (system::is_add_overflow<size_t>(buffer_.size(), chunk))
        return chunk_storage::eof;
    if (buffer_.size() + chunk > buffer_.max_size())
        return chunk_storage::eof;

    std::unique_lock map_lock(map_mutex_);
    const auto link = buffer_.size();
    buffer_.resize(buffer_.size() + chunk, 0x00);
    return link;
}

memory_ptr chunk_storage::set(size_t offset, size_t size,
    uint8_t backfill) NOEXCEPT
{
    {
        std::unique_lock field_lock(field_mutex_);
        if (system::is_add_overflow(offset, size))
        {
            return {};
        }
        else
        {
            std::unique_lock map_lock(map_mutex_);
            const auto minimum = offset + size;
            if (minimum > buffer_.size())
                buffer_.resize(minimum, backfill);
        }
    }

    return get(offset);
}

memory_ptr chunk_storage::get(size_t offset) const NOEXCEPT
{
    const auto ptr = std::make_shared<accessor<std::shared_mutex>>(map_mutex_);

    // With offset > size the assignment is negative (stream is exhausted).
    ptr->assign(get_raw(offset), get_raw(size()));

    return ptr;
}

memory::iterator chunk_storage::get_raw(size_t offset) const NOEXCEPT
{
    return std::next(buffer_.data(), offset);
}

code chunk_storage::get_fault() const NOEXCEPT
{
    return {};
}

size_t chunk_storage::get_space() const NOEXCEPT
{
    return {};
}

BC_POP_WARNING()

} // namespace test
