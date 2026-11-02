/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#if !defined(HAVE_WINDOWS)
    #include <fcntl.h>
    #include <unistd.h>
#endif
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/locks/file_lock.hpp>

namespace libbitcoin {
namespace database {

#if defined(HAVE_WINDOWS)
static void* const invalid = INVALID_HANDLE_VALUE;
#else
static constexpr int invalid = -1;
#endif

// Open the existing file and take an exclusive lock on its whole extent.
// Sharing allows the owner to delete the file before closing it.
static auto open_locked(const std::filesystem::path& file) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto path = system::extended_path(file);
    BC_POP_WARNING()

#if defined(HAVE_WINDOWS)
    constexpr auto share = FILE_SHARE_READ | FILE_SHARE_WRITE |
        FILE_SHARE_DELETE;
    const auto handle = ::CreateFileW(path.c_str(), GENERIC_READ |
        GENERIC_WRITE, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (handle == invalid)
        return invalid;

    constexpr auto flags = LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY;
    OVERLAPPED overlapped{};
    if (::LockFileEx(handle, flags, 0, MAXDWORD, MAXDWORD, &overlapped) ==
        FALSE)
    {
        ::CloseHandle(handle);
        return invalid;
    }

    return handle;
#else
    const auto descriptor = ::open(path.c_str(), O_RDWR);
    if (descriptor == invalid)
        return invalid;

    struct flock lock{};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    if (::fcntl(descriptor, F_SETLK, &lock) == -1)
    {
        ::close(descriptor);
        return invalid;
    }

    return descriptor;
#endif
}

// Closing releases the lock.
template <typename Handle>
static void close_locked(Handle handle) NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    ::CloseHandle(handle);
#else
    ::close(handle);
#endif
}

interprocess_lock::interprocess_lock(const std::filesystem::path& file) NOEXCEPT
  : file_lock(file), handle_(invalid)
{
}

interprocess_lock::~interprocess_lock() NOEXCEPT
{
    try_unlock();
}

// Lock is not idempotent, returns false if already locked (or error).
// This succeeds if no other process has exclusive or sharable ownership.
bool interprocess_lock::try_lock() NOEXCEPT
{
    // A valid handle guarantees file existence and ownership.
    if (handle_ != invalid)
        return false;

    // Create the file.
    if (!create())
        return false;

    // Obtain exclusive access to the file.
    handle_ = open_locked(file());
    return handle_ != invalid;
}

// Unlock is idempotent, returns true if unlocked on return (or success).
// This may leave the lock file behind, which is not a problem.
bool interprocess_lock::try_unlock() NOEXCEPT
{
    // An invalid handle guarantees lack of ownership, but file may exist.
    // Do not delete the file unless we own it.
    if (handle_ == invalid)
        return true;

    // Delete before close, to preclude delete of a file that is not owned,
    // resulting from a race condition. The file is queued for deletion.
    const auto result = destroy();

    // Release exclusive access to the file.
    close_locked(handle_);
    handle_ = invalid;
    return result;
}

} // namespace database
} // namespace libbitcoin
