/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/concurrent/flush_lock.hpp>

#include <boost/filesystem.hpp>

namespace libbitcoin {
namespace network {

flush_lock::flush_lock(const boost::filesystem::path& file) noexcept
  : file_lock(file)
{
}

bool flush_lock::try_lock() const noexcept
{
    return !exists();
}

// This is non-const as it alters state (externally but may become internal).
bool flush_lock::lock() noexcept
{
    if (!try_lock())
        return false;

    return create();
}

// This is non-const as it alters state (externally but may become internal).
bool flush_lock::unlock() noexcept
{
    if (try_lock())
        return false;

    return destroy();
}

} // namespace network
} // namespace libbitcoin
