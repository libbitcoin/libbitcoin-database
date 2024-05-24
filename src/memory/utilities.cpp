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
#include <bitcoin/database/memory/utilities.hpp>

#if defined(HAVE_MSC)
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

#if defined(HAVE_MSC)

uint64_t system_memory() NOEXCEPT
{
    // learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/
    // nf-sysinfoapi-globalmemorystatusex
    // "If the function fails, the return value is zero."
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    return GlobalMemoryStatusEx(&status) == zero ? zero :
        status.ullTotalPhys;
}

#else

uint64_t system_memory() NOEXCEPT
{
    using namespace system;

    // man7.org/linux/man-pages/man3/sysconf.3.html
    // "It is possible for the product of these values to overflow."
    const int64_t pages = sysconf(_SC_PHYS_PAGES);
    const int64_t sizes = sysconf(_SC_PAGE_SIZE);
    return (is_negative(pages) || is_negative(sizes)) ? zero :
        ceilinged_multiply(to_unsigned(pages), to_unsigned(sizes));
}

#endif

} // namespace database
} // namespace libbitcoin
