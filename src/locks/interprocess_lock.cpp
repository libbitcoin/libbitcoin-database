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
#include <bitcoin/database/locks/interprocess_lock.hpp>

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/locks/file_lock.hpp>

namespace libbitcoin {
namespace database {

// ipcdetail functions do not throw (but are unannotated).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// construct/destruct
// ----------------------------------------------------------------------------

interprocess_lock::interprocess_lock(const std::filesystem::path& file) NOEXCEPT
  : file_lock(file), handle_(invalid)
{
    // This is a behavior change, we no longer open file (or throw) here.
}

interprocess_lock::~interprocess_lock() NOEXCEPT
{
    unlock();
}

// public
// ----------------------------------------------------------------------------

// Lock is not idempotent, returns false if already locked (or error).
// This succeeds if no other process has exclusive or sharable ownership.
bool interprocess_lock::lock() NOEXCEPT
{
    // A valid handle guarantees file existence and ownership.
    if (handle_ != invalid)
        return false;

    // Create the file.
    if (!create())
        return false;

    // Get a handle to the file.
    const auto handle = open_existing_file(file());
    bool result;

    // Obtain exclusive access to the file.
    if (ipcdetail::try_acquire_file_lock(handle, result) && result)
    {
        handle_ = handle;
        return true;
    }

    handle_ = invalid;
    return false;
}

// Unlock is idempotent, returns true if unlocked on return (or success).
// This may leave the lock file behind, which is not a problem.
bool interprocess_lock::unlock() noexcept
{
    // An invalid handle guarantees lack of ownership, but file may exist.
    // Do not delete the file unless we own it.
    if (handle_ == invalid)
        return true;

    // Delete before close, to preclude delete of a file that is not owned,
    // resulting from a race condition. The file is queued for deletion.
    const auto result = destroy();

    // Release exclusive access to the file.
    ipcdetail::close_file(handle_);
    handle_ = invalid;
    return result;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
