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
#include <bitcoin/database/file/utilities.hpp>

#if defined(HAVE_MSC)
    #include <io.h>
#endif
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
namespace file {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

inline path trim(const path& value) NOEXCEPT
{
    // Trailing slash may cause successful create_directories returning false.
    return system::trim_right_copy(value.string(), { "/", "\\", "\x20" });
}

bool is_directory(const path& directory) NOEXCEPT
{
    std::error_code ununsed;
    const auto path = system::to_extended_path(directory);
    return std::filesystem::is_directory(path, ununsed);
}

bool clear_directory(const path& directory) NOEXCEPT
{
    std::error_code ec;
    const auto path = system::to_extended_path(directory);
    std::filesystem::remove_all(path, ec);
    return (!ec && std::filesystem::create_directories(path, ec)) || !ec;
}

bool create_directory(const path& directory) NOEXCEPT
{
    std::error_code unused;
    const auto path = system::to_extended_path(trim(directory));
    return std::filesystem::create_directories(path, unused);
}

bool is_file(const path& filename) NOEXCEPT
{
    system::ifstream file(filename);
    const auto good = file.good();
    file.close();
    return good;
}

bool create_file(const path& filename) NOEXCEPT
{
    system::ofstream file(filename);
    const auto good = file.good();
    file.close();
    return good;
}

bool create_file(const path& to, const uint8_t* data, size_t size) NOEXCEPT
{
    // Binary mode on Windows ensures that \n not replaced with \r\n.
    system::ofstream file(to, std::ios_base::binary);
    if (!file.good()) return false;
    file.write(pointer_cast<const char>(data), size);
    if (!file.good()) return false;
    file.close();
    return file.good();
}

// directory|file
bool remove(const path& name) NOEXCEPT
{
    std::error_code ec;
    const auto path = system::to_extended_path(name);

    // False if did not exist, but in that case there is no error code.
    return std::filesystem::remove(path, ec) || !ec;
}

// directory|file
bool rename(const path& from, const path& to) NOEXCEPT
{
    // en.cppreference.com/w/cpp/filesystem/rename
    std::error_code ec;
    const auto from_path = system::to_extended_path(from);
    const auto to_path = system::to_extended_path(to);
    std::filesystem::rename(from_path, to_path, ec);
    return !ec;
}

// File descriptor functions required for memory mapping.

int open(const path& filename) NOEXCEPT
{
    const auto path = system::to_extended_path(filename);

    // _wsopen_s and wstring do not throw (but are unannotated).
#if defined(HAVE_MSC)
    int file_descriptor;
    if (_wsopen_s(&file_descriptor, path.c_str(),
        O_RDWR | _O_BINARY | _O_RANDOM, _SH_DENYWR,
        _S_IREAD | _S_IWRITE) == -1)
        file_descriptor = -1;
#else
    int file_descriptor = ::open(path.c_str(),
        O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
    return file_descriptor;
}

bool close(int file_descriptor) NOEXCEPT
{
    // TODO: it may be possible to collapse this given newer WIN32 API.
#if defined(HAVE_MSC)
    return is_zero(_close(file_descriptor));
#else
    return is_zero(::close(file_descriptor));
#endif
}

bool size(size_t& out, int file_descriptor) NOEXCEPT
{
    if (file_descriptor == -1)
        return false;

    // TODO: it may be possible to collapse this given newer WIN32 API.
    // This is required because off_t is defined as long, which is 32|64 bits
    // in msvc and 64 bits in linux/osx, and stat contains off_t.
#if defined(HAVE_MSC)
#if defined(HAVE_X64)
    struct _stat64 sbuf;
    if (_fstat64(file_descriptor, &sbuf) == -1)
        return false;
#else
    struct _stat32 sbuf;
    if (_fstat32(file_descriptor, &sbuf) == -1)
        return false;
#endif
#else
    // Limited to 32 bit files on 32 bit systems, see linux.die.net/man/2/open
    struct stat sbuf;
    if (fstat(file_descriptor, &sbuf) == -1)
        return false;
#endif
    if (is_limited<size_t>(sbuf.st_size))
        return false;

    out = sign_cast<size_t>(sbuf.st_size);
    return true;
}

size_t page() NOEXCEPT
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

bool size(size_t& out, const std::filesystem::path& file_path) NOEXCEPT
{
    code ec;
    const auto size = std::filesystem::file_size(
        to_extended_path(file_path), ec);

    if (ec || is_limited<size_t>(size))
        return false;

    out = possible_narrow_and_sign_cast<size_t>(size);
    return true;
}

BC_POP_WARNING()

} // namespace file
} // namespace database
} // namespace libbitcoin
