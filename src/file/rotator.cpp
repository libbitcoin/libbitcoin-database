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

#include <exception>
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
    BC_ASSERT_MSG(!stream_, "rotator not stopped");
    return !stream_ && set_size() && set_stream();
}

bool rotator::stop() NOEXCEPT
{
    if (!stream_)
        return false;

    flush();
    stream_.reset();
    return true;
}

bool rotator::write(const std::string& message) NOEXCEPT
{
    if (!stream_)
        return false;

    const auto size = message.size();
    if (size >= limit_)
        return false;

    size_ = system::ceilinged_add(size, size_);
    if (size_ >= limit_)
    {
        if (!rotate())
            return false;

        size_ = size;
    }

    try
    {
        *stream_ << message;
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

bool rotator::flush() NOEXCEPT
{
    if (!stream_)
        return false;

    try
    {
        stream_->flush();
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

// protected
bool rotator::rotate() NOEXCEPT
{
    return stop()
        && file::remove(path2_)
        && file::rename(path1_, path2_)
        && start();
}

// protected
bool rotator::set_size() NOEXCEPT
{
    size_ = zero;
    return !file::is_file(path1_) || file::size(size_, path1_);
}

// protected
bool rotator::set_stream() NOEXCEPT
{
    // Binary mode on Windows ensures that \n is not replaced with \r\n.
    constexpr auto mode = std::ios_base::app | std::ios_base::binary;

    try
    {
        stream_ = std::make_shared<system::ofstream>(path1_, mode);
        return stream_ && stream_->good();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

} // namespace file
} // namespace database
} // namespace libbitcoin
