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
#include <bitcoin/database/locks/flush_lock.hpp>

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
flush_lock::flush_lock(const std::filesystem::path& file) NOEXCEPT
  : file_lock(file)
{
}

bool flush_lock::try_lock() const NOEXCEPT
{
    return !exists();
}

// This is non-const as it alters state (externally but may become internal).
bool flush_lock::lock() NOEXCEPT
{
    if (!try_lock())
        return false;

    return create();
}

// This is non-const as it alters state (externally but may become internal).
bool flush_lock::unlock() NOEXCEPT
{
    if (try_lock())
        return false;

    return destroy();
}

} // namespace database
} // namespace libbitcoin
