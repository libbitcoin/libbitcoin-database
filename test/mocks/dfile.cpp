/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include "../mocks/dfile.hpp"
#include <filesystem>
#include <mutex>
#include <shared_mutex>

namespace test {

// locks may throw.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// This is a trivial working dfile interface implementation.
dfile::dfile() NOEXCEPT
  : path_{}, local_{}, buffer_{ local_ }
{
}

dfile::dfile(system::data_chunk& reference) NOEXCEPT
  : path_{}, local_{}, buffer_{ reference }
{
}

dfile::dfile(const std::filesystem::path& filename, size_t, size_t) NOEXCEPT
  : path_{ filename }, local_{}, buffer_{ local_ }
{
}

system::data_chunk& dfile::buffer() NOEXCEPT
{
    return buffer_;
}

code dfile::open() NOEXCEPT
{
    return error::success;
}

code dfile::close() NOEXCEPT
{
    return error::success;
}

code dfile::load() NOEXCEPT
{
    return error::success;
}

code dfile::flush() const NOEXCEPT
{
    return error::success;
}

code dfile::unload() NOEXCEPT
{
    return error::success;
}

const std::filesystem::path& dfile::file() const NOEXCEPT
{
    return path_;
}

size_t dfile::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.capacity();
}

size_t dfile::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return buffer_.size();
}

bool dfile::truncate(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);
    if (size > buffer_.size())
        return false;

    buffer_.resize(size);
    return true;
}

size_t dfile::allocate(size_t chunk) NOEXCEPT
{
    if (system::is_add_overflow<size_t>(buffer_.size(), chunk))
        return dfile::eof;

    if (buffer_.size() + chunk > buffer_.max_size())
        return dfile::eof;

    std::unique_lock field_lock(field_mutex_);
    std::unique_lock map_lock(map_mutex_);
    const auto link = buffer_.size();
    buffer_.resize(buffer_.size() + chunk);
    return link;
}

memory_ptr dfile::get(size_t offset) const NOEXCEPT
{
    const auto ptr = std::make_shared<accessor<std::shared_mutex>>(map_mutex_);

    // With offset > size the assignment is negative (stream is exhausted).
    ptr->assign(
        std::next(buffer_.data(), offset),
        std::next(buffer_.data(), size()));

    return ptr;
}

BC_POP_WARNING()

} // namespace test
