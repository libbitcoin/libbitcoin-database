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

#else

size_t page_size() NOEXCEPT
{
    using namespace system;
    errno = 0;

    const auto size = sysconf(_SC_PAGESIZE);
    if (is_negative(size) || is_nonzero(errno))
        return zero;

    BC_ASSERT(!system::is_limited<size_t>(size));
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
    return ceilinged_multiply(to_unsigned(pages), page_size());
}

#endif

} // namespace database
} // namespace libbitcoin
