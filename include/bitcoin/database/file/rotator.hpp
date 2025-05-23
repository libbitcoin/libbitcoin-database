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
#ifndef LIBBITCOIN_DATABASE_FILE_ROTATOR_HPP
#define LIBBITCOIN_DATABASE_FILE_ROTATOR_HPP

#include <memory>
#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/utilities.hpp>

namespace libbitcoin {
namespace database {
namespace file {

/// Because device requires std::ofstream::value_type.
struct ofstream_wrap
{
    using value_type = std::ofstream::char_type;
    std::ofstream& stream_;
};

/// Simple two file rotating stream with configurable size and file names.
/// Files are rotated after full and writes are contiguous across them.
class BCD_API rotator_sink
  : public system::device<ofstream_wrap>
{
public:
    using path = std::filesystem::path;
    struct category
      : system::ios::sink_tag,
        system::ios::flushable_tag,
        system::ios::optimally_buffered_tag
    {
    };

    rotator_sink(const path& path1, const path& path2, size_t limit) NOEXCEPT;
    size_type write(const char_type* buffer, size_type count) THROWS;
    bool flush() THROWS;

protected:
    size_type do_optimal_buffer_size() const NOEXCEPT override;

    bool start() NOEXCEPT;
    bool stop() NOEXCEPT;
    bool rotate() NOEXCEPT;
    bool set_remaining() NOEXCEPT;
    bool set_stream() NOEXCEPT;

private:
    // These are thread safe.
    const path path1_;
    const path path2_;
    const size_type limit_;

    // This is not thread safe.
    std::shared_ptr<system::ofstream> stream_{};
};

namespace stream
{
    namespace out
    {
        using rotator = system::make_stream<rotator_sink>;
    }
}

} // namespace file
} // namespace database
} // namespace libbitcoin

#endif
