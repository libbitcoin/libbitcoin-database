/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/file/rotator.hpp>

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace file {

rotator::rotator(const path& path1, const path& path2, size_t limit) NOEXCEPT
  : path1_(path1), path2_(path2), limit_(limit)
{
}

bool rotator::start() NOEXCEPT
{
    return set_size() && set_stream();
}

void rotator::stop() NOEXCEPT
{
    stream_->flush();
    stream_.reset();
}

bool rotator::write(const std::string& message) NOEXCEPT
{
    if (!stream_)
        return false;

    const auto size = message.size();
    size_ = system::ceilinged_add(size, size_);
    if (size_ >= limit_)
    {
        if (!rotate())
            return false;

        size_ = size;
    }

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    *stream_ << message;
    BC_POP_WARNING()
    return true;
}

// protected
bool rotator::set_size() NOEXCEPT
{
    size_ = zero;
    const auto handle = file::open(path1_);
    if (handle == invalid)
        return false;

    size_ = file::size(handle);
    return file::close(handle);
}

// protected
bool rotator::set_stream() NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    stream_ = std::make_shared<system::ofstream>(path1_);
    return stream_ && stream_->good();
    BC_POP_WARNING()
}

// protected
bool rotator::rotate() NOEXCEPT
{
    stop();
    return file::remove(path2_) && file::rename(path1_, path2_) && start();
}

} // namespace file
} // namespace database
} // namespace libbitcoin
