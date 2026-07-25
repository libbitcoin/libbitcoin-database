/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <algorithm>
#include <cerrno>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>

#if defined(MANAGE_STAGING)

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>

size_t system_pressure() NOEXCEPT
{
    int level{};
    auto size = sizeof(level);
    if (::sysctlbyname("kern.memorystatus_vm_pressure_level", &level, &size,
        nullptr, 0) != 0)
        return {};

    return static_cast<size_t>(level);
}


void* mmap_reserve(size_t size) NOEXCEPT
{
    return ::mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);
}

int mmap_commit(void* address, size_t size) NOEXCEPT
{
    return ::mprotect(address, size, PROT_READ | PROT_WRITE);
}

int mmap_settle(void* address, size_t size, int fd, size_t offset) NOEXCEPT
{
    return ::mmap(address, size, PROT_READ, MAP_SHARED | MAP_FIXED, fd,
        static_cast<off_t>(offset)) == MAP_FAILED ? -1 : 0;
}

int mmap_unsettle(void* address, size_t size) NOEXCEPT
{
    return ::mmap(address, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED ? -1 : 0;
}

// Chunked to avoid platform-specific single-transfer length limits.
constexpr size_t transfer_chunk = size_t{ 1 } << 30;

bool pread_all(int fd, uint8_t* to, size_t size, size_t offset) NOEXCEPT
{
    while (size > 0)
    {
        const auto request = std::min(size, transfer_chunk);
        const auto result = ::pread(fd, to, request,
            static_cast<off_t>(offset));

        if (result < 0)
        {
            if (errno == EINTR)
                continue;

            return false;
        }

        // Early eof implies the requested range exceeds the file.
        if (result == 0)
            return false;

        const auto bytes = static_cast<size_t>(result);
        to += bytes;
        offset += bytes;
        size -= bytes;
    }

    return true;
}

bool pwrite_all(int fd, const uint8_t* from, size_t size,
    size_t offset) NOEXCEPT
{
    while (size > 0)
    {
        const auto request = std::min(size, transfer_chunk);
        const auto result = ::pwrite(fd, from, request,
            static_cast<off_t>(offset));

        if (result <= 0)
        {
            if (result < 0 && errno == EINTR)
                continue;

            return false;
        }

        const auto bytes = static_cast<size_t>(result);
        from += bytes;
        offset += bytes;
        size -= bytes;
    }

    return true;
}

#endif // MANAGE_STAGING
