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
#include <bitcoin/database/memory/utilities.hpp>

#if defined(HAVE_MSC)
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#if defined(HAVE_APPLE)
    #include <mach/mach.h>
    #include <sys/sysctl.h>
#endif
#if defined(HAVE_LINUX)
    #include <algorithm>
    #include <cstdio>
#endif
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

#if defined(HAVE_MSC)

size_t page_size() NOEXCEPT
{
    using namespace system;
    SYSTEM_INFO info{};
    ::GetSystemInfo(&info);

    BC_ASSERT(!system::is_limited<size_t>(info.dwPageSize));
    return info.dwPageSize;
}

uint64_t system_memory() NOEXCEPT
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    return is_zero(::GlobalMemoryStatusEx(&status)) ? zero :
        status.ullTotalPhys;
}

uint64_t system_free() NOEXCEPT
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    return is_zero(::GlobalMemoryStatusEx(&status)) ? zero :
        status.ullAvailPhys;
}

size_t system_pressure() NOEXCEPT
{
    // The kernel low memory resource signal (no configurable threshold).
    BOOL low{ FALSE };
    const auto handle = ::CreateMemoryResourceNotification(
        ::LowMemoryResourceNotification);

    if (handle == NULL)
        return zero;

    const auto success = !is_zero(::QueryMemoryResourceNotification(handle,
        &low));
    ::CloseHandle(handle);

    return success ? (low == FALSE ? 1_size : 4_size) : zero;
}

uint64_t system_compressed() NOEXCEPT
{
    // No cheap source (performance counters only).
    return zero;
}

#else

size_t page_size() NOEXCEPT
{
    errno = 0;
    using namespace system;
    const auto size = sysconf(_SC_PAGESIZE);
    if (is_zero(errno) && !is_negative(size))
    {
        BC_ASSERT(!is_limited<size_t>(size));
        return possible_narrow_sign_cast<size_t>(size);
    }

    return zero;
}

uint64_t system_memory() NOEXCEPT
{
    errno = 0;
    using namespace system;
    const int64_t pages = sysconf(_SC_PHYS_PAGES);
    if (is_zero(errno) && !is_negative(pages))
    {
        // Failed page_size also results in zero return.
        const auto size = possible_wide_cast<uint64_t>(page_size());
        return ceilinged_multiply(to_unsigned(pages), size);
    }

    return zero;
}

uint64_t system_free() NOEXCEPT
{
#if defined(HAVE_APPLE)
    using namespace system;
    auto count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t statistics{};
    if (::host_statistics64(::mach_host_self(), HOST_VM_INFO64,
        pointer_cast<integer_t>(&statistics), &count) == KERN_SUCCESS)
    {
        return ceilinged_multiply<uint64_t>(statistics.free_count, page_size());
    }
#else
    errno = 0;
    using namespace system;
    const int64_t pages = sysconf(_SC_AVPHYS_PAGES);
    if (is_zero(errno) && !is_negative(pages))
    {
        // Failed page_size() also results in zero return.
        const auto size = possible_wide_cast<uint64_t>(page_size());
        return ceilinged_multiply(to_unsigned(pages), size);
    }
#endif

    // Failed or no platform source.
    return zero;
}

size_t system_pressure() NOEXCEPT
{
#if defined(HAVE_APPLE)
    int level{};
    auto size = sizeof(level);
    if (is_zero(::sysctlbyname("kern.memorystatus_vm_pressure_level", &level,
        &size, nullptr, 0)))
    {
        return system::possible_narrow_sign_cast<size_t>(level);
    }
#elif defined(HAVE_LINUX)
    // PSI (requires CONFIG_PSI): fraction of recent wall time that tasks
    // stalled on memory reclaim. A tenth of time stalled is treated as genuine
    // pressure, partial (some) as warning and total (full) as critical,
    // mapping to the macos memorystatus level semantics.
    if (const auto file = std::fopen("/proc/pressure/memory", "r"))
    {
        char line[128];
        auto level = one;
        double average10{};
        while (!is_null(std::fgets(line, sizeof(line), file)))
        {
            if ((std::sscanf(line, "some average10=%lf", &average10) == 1) &&
                (average10 >= 10.0))
                level = std::max(level, 2_size);

            if ((std::sscanf(line, "full average10=%lf", &average10) == 1) &&
                (average10 >= 10.0))
                level = std::max(level, 4_size);
        }

        std::fclose(file);
        return level;
    }
#endif

    // Failed or no platform source.
    return zero;
}

uint64_t system_compressed() NOEXCEPT
{
#if defined(HAVE_APPLE)
    using namespace system;
    auto count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t statistics{};
    if (::host_statistics64(::mach_host_self(), HOST_VM_INFO64,
        pointer_cast<integer_t>(&statistics), &count) == KERN_SUCCESS)
    {
        const auto pages = statistics.compressor_page_count;
        return ceilinged_multiply<uint64_t>(pages, page_size());
    }
#endif

    // Failed or no platform source adopted. Linux zswap/zram accounting is
    // configuration dependent, PSI carries the pressure signal there.
    return zero;
}

#endif

} // namespace database
} // namespace libbitcoin
