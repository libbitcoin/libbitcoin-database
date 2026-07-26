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
    GetSystemInfo(&info);

    BC_ASSERT(!system::is_limited<size_t>(info.dwPageSize));
    return info.dwPageSize;
}

uint64_t system_memory() NOEXCEPT
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    return is_zero(GlobalMemoryStatusEx(&status)) ? zero :
        status.ullTotalPhys;
}

size_t system_pressure() NOEXCEPT
{
    // The kernel low memory resource signal (no configurable threshold).
    BOOL low{ FALSE };
    const auto handle = CreateMemoryResourceNotification(
        LowMemoryResourceNotification);

    if (handle == NULL)
        return zero;

    const auto success = QueryMemoryResourceNotification(handle, &low) != 0;
    CloseHandle(handle);
    return !success ? zero : (low == FALSE ? size_t{ 1 } : size_t{ 4 });
}

#else

size_t page_size() NOEXCEPT
{
    using namespace system;
    errno = 0;

    const auto size = sysconf(_SC_PAGESIZE);
    if (is_negative(size) || is_nonzero(errno))
        return zero;

    BC_ASSERT(!is_limited<size_t>(size));
    return possible_narrow_sign_cast<size_t>(size);
}

uint64_t system_memory() NOEXCEPT
{
    using namespace system;
    errno = 0;

    const int64_t pages = sysconf(_SC_PHYS_PAGES);
    if (is_negative(pages) || is_nonzero(errno))
        return zero;

    // Failed page_size also results in zero return.
    return ceilinged_multiply(to_unsigned(pages),
        possible_wide_cast<uint64_t>(page_size()));
}

size_t system_pressure() NOEXCEPT
{
#if defined(HAVE_APPLE)
    using namespace system;
    int level{};
    auto size = sizeof(level);
    if (::sysctlbyname("kern.memorystatus_vm_pressure_level", &level, &size,
        nullptr, 0) != 0)
        return zero;

    return possible_narrow_sign_cast<size_t>(level);
#elif defined(HAVE_LINUX)
    // PSI (requires CONFIG_PSI): fraction of recent wall time that tasks
    // stalled on memory reclaim. A tenth of time stalled is treated as
    // genuine pressure, partial (some) as warning and total (full) as
    // critical, mapping to the macos memorystatus level semantics.
    auto level = zero;
    if (const auto file = std::fopen("/proc/pressure/memory", "r"))
    {
        char line[128];
        double avg10{};
        level = one;
        while (!is_null(std::fgets(line, sizeof(line), file)))
        {
            if ((std::sscanf(line, "some avg10=%lf", &avg10) == 1) &&
                (avg10 >= 10.0))
                level = std::max(level, size_t{ 2 });

            if ((std::sscanf(line, "full avg10=%lf", &avg10) == 1) &&
                (avg10 >= 10.0))
                level = std::max(level, size_t{ 4 });
        }

        std::fclose(file);
    }

    return level;
#else
    // No platform source.
    return zero;
#endif
}

#endif

} // namespace database
} // namespace libbitcoin
