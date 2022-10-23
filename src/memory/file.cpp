/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/memory/file.hpp>

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

using namespace system;
using namespace std::filesystem;

bool clear_path(const path& directory) NOEXCEPT
{
    // remove_all returns count removed, and error code if fails.
    // create_directories returns true if path exists or created.
    // used for setup, with no expectations of file/directory existence.
    std::error_code ec;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    remove_all(directory, ec);
    return !ec && create_directories(directory, ec);
    BC_POP_WARNING()
}

bool create_file(const path& filename) NOEXCEPT
{
    // Creates and returns true if file already existed (and no error).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ofstream file(filename);
    const auto good = file.good();
    file.close();
    BC_POP_WARNING()
    return good;
}

bool file_exists(const path& filename) NOEXCEPT
{
    // Returns true only if file existed.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ifstream file(filename);
    const auto good = file.good();
    file.close();
    BC_POP_WARNING()
    return good;
}

bool remove_file(const path& filename) NOEXCEPT
{
    // Deletes and returns false if file did not exist (or error).
    std::error_code ec;
    return remove(filename, ec);
}

bool rename_file(const path& from, const path& to) NOEXCEPT
{
    // en.cppreference.com/w/cpp/filesystem/rename
    std::error_code ec;
    rename(from, to, ec);
    return !ec;
}

// File descriptor functions required for memory mapping.

int open_file(const path& filename) NOEXCEPT
{
    // _wsopen_s and wstring do not throw (but are unannotated).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
#if defined(HAVE_MSC)
    int file_descriptor;
    if (_wsopen_s(&file_descriptor, filename.wstring().c_str(),
        (O_RDWR | _O_BINARY | _O_RANDOM), _SH_DENYWR,
        (_S_IREAD | _S_IWRITE)) == -1)
        file_descriptor = -1;
#else
    int file_descriptor = ::open(filename.string().c_str(),
        (O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
#endif
    return file_descriptor;
    BC_POP_WARNING()
}

bool close_file(int file_descriptor) NOEXCEPT
{
    // TODO: it may be possible to collapse this given newer WIN32 API.
#if defined(HAVE_MSC)
    return is_zero(_close(file_descriptor));
#else
    return is_zero(::close(file_descriptor));
#endif
}

size_t file_size(int file_descriptor) NOEXCEPT
{
    if (file_descriptor == -1)
        return zero;

    // TODO: it may be possible to collapse this given newer WIN32 API.
    // This is required because off_t is defined as long, which is 32|64 bits
    // in msvc and 64 bits in linux/osx, and stat contains off_t.
#if defined(HAVE_MSC)
#if defined(HAVE_X64)
    struct _stat64 sbuf;
    if (_fstat64(file_descriptor, &sbuf) == -1)
        return zero;
#else
    struct _stat32 sbuf;
    if (_fstat32(file_descriptor, &sbuf) == -1)
        return zero;
#endif
#else
    // Limited to 32 bit files on 32 bit systems, see linux.die.net/man/2/open
    struct stat sbuf;
    if (fstat(file_descriptor, &sbuf) == -1)
        return zero;
#endif

    return sign_cast<size_t>(sbuf.st_size);
}

size_t page_size() NOEXCEPT
{
#if defined(HAVE_MSC)
    SYSTEM_INFO configuration;
    GetSystemInfo(&configuration);
    return configuration.dwPageSize;
#else
    errno = 0;
    const auto page_size = sysconf(_SC_PAGESIZE);

    if (is_negative(page_size) || !is_zero(errno))
        return 0;

    BC_ASSERT(possible_narrow_sign_cast<uint64_t>(page_size) <= max_size_t);
    return possible_narrow_sign_cast<size_t>(page_size);
#endif
}

} // namespace database
} // namespace libbitcoin
