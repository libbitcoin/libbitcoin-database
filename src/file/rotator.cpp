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
#include <bitcoin/database/file/rotator.hpp>

#include <exception>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace file {
    
// If start fails here, first write will throw, invalidating outer stream.
rotator_sink::rotator_sink(const path& path1, const path& path2,
    size_t limit) NOEXCEPT
  : device<ofstream_wrap>({}), path1_(path1), path2_(path2), limit_(limit)
{
    start();
}

// methods
// ----------------------------------------------------------------------------
// www.boost.org/doc/libs/1_79_0/libs/iostreams/doc/guide/exceptions.html
// Must throw to invalidate the stream (system streams only indicate eof).

BC_PUSH_WARNING(NO_METHOD_HIDING)
typename rotator_sink::size_type
rotator_sink::write(const char_type* buffer, size_type count) THROWS
BC_POP_WARNING()
{
    if (is_null(buffer))
    {
        throw ostream_exception{ "buffer" };
    }
    else if (system::is_negative(count))
    {
        throw ostream_exception{ "count" };
    }
    else if (system::is_negative(remaining_))
    {
        throw ostream_exception{ "remaining" };
    }
    else if (!stream_)
    {
        throw ostream_exception{ "stream" };
    }

    // Always write it all, since it's a circular buffer.
    const auto written = count;

    while (is_nonzero(count))
    {
        if (is_zero(remaining_) && !rotate())
            throw ostream_exception{ "rotate" };

        const auto size = std::min(remaining_, count);
        stream_->write(buffer, size);
        std::advance(buffer, size);
        remaining_ -= size;
        count -= size;
    }

    return written;
}

bool rotator_sink::flush() THROWS
{
    if (!stream_)
        return false;

    stream_->flush();
    return true;
}

// protected
// ----------------------------------------------------------------------------

bool rotator_sink::start() NOEXCEPT
{
    return !stream_ && set_remaining() && set_stream();
}

bool rotator_sink::stop() NOEXCEPT
{
    if (!stream_)
        return false;

    try
    {
        stream_->flush();
        stream_.reset();
        return true;
    }
    catch (const std::exception&)
    {
        stream_.reset();
        return false;
    }
}

bool rotator_sink::rotate() NOEXCEPT
{
    if (stop() &&
        file::remove(path2_) &&
        file::rename(path1_, path2_) &&
        start())
    {
        remaining_ = limit_;
        return true;
    }

    remaining_ = {};
    return false;
}

bool rotator_sink::set_remaining() NOEXCEPT
{
    if (!file::is_file(path1_))
    {
        remaining_ = limit_;
        return true;
    }

    size_t out{};
    remaining_ = {};
    if (!file::size(out, path1_))
        return false;

    if (system::is_limited<size_type>(out))
        return true;

    const auto size = system::possible_narrow_sign_cast<size_type>(out);
    if (size >= limit_)
        return true;

    remaining_ = limit_ - size;
    return true;
}

bool rotator_sink::set_stream() NOEXCEPT
{
    // Binary mode on Windows ensures that \n is not replaced with \r\n.
    constexpr auto mode = std::ios_base::app | std::ios_base::binary;

    try
    {
        stream_ = std::make_shared<system::ofstream>(path1_, mode);
        if (stream_ && stream_->good())
            return true;
    }
    catch (const std::exception&)
    {
    }

    stream_.reset();
    return false;
}

typename rotator_sink::size_type
rotator_sink::do_optimal_buffer_size() const NOEXCEPT
{
    return std::min(limit_, device<ofstream_wrap>::do_optimal_buffer_size());
}

} // namespace file
} // namespace database
} // namespace libbitcoin
