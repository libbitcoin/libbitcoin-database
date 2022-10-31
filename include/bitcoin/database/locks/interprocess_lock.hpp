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
#ifndef LIBBITCOIN_DATABASE_LOCKS_INTERPROCESS_LOCK_HPP
#define LIBBITCOIN_DATABASE_LOCKS_INTERPROCESS_LOCK_HPP

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/locks/file_lock.hpp>

namespace libbitcoin {
namespace database {

/// This class is not thread safe, and does not throw.
class BCD_API interprocess_lock final
  : file_lock
{
public:
    DELETE4(interprocess_lock);

    /// Construction does not touch the file.
    interprocess_lock(const std::filesystem::path& file) NOEXCEPT;

    /// Destruction calls unlock.
    ~interprocess_lock() NOEXCEPT;

    /// Creates the file and acquires exclusive access.
    /// Returns false if failed to acquire lock or lock already held.
    bool lock() NOEXCEPT;

    /// Releases access (if locked) and deletes the file.
    /// Returns true if lock not held or succesfully unlocked and deleted.
    bool unlock() NOEXCEPT;

private:
    file_handle_t handle_;
};

} // namespace database
} // namespace libbitcoin

#endif
