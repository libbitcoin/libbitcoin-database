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

/// Not thread safe.
/// Simple two file log rotator with configurable size and file names.
class BCD_API rotator
{
public:
    using path = std::filesystem::path;

    DELETE_COPY_MOVE(rotator);

    /// Construct log rotator with paths and size limit.
    rotator(const path& path1, const path& path2, size_t limit) NOEXCEPT;

    /// Start rotator, should be stopped.
    bool start() NOEXCEPT;

    /// Stop and flush rotator, should be started.
    bool stop() NOEXCEPT;

    /// Write message to log.
    bool write(const std::string& message) NOEXCEPT;

protected:
    bool rotate() NOEXCEPT;
    bool set_size() NOEXCEPT;
    bool set_stream() NOEXCEPT;

private:
    // These are thread safe.
    const path path1_;
    const path path2_;
    const size_t limit_;

    // These are not thread safe.
    std::shared_ptr<system::ofstream> stream_{};
    size_t size_{};
};

} // namespace file
} // namespace database
} // namespace libbitcoin

#endif
