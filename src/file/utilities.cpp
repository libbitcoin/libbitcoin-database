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
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {
namespace file {

using namespace system;

// Many return values are redundant with return codes.
// There is no reason to obtain the return value of these non-discardables.
BC_PUSH_WARNING(DISCARDING_NON_DISCARDABLE)
BC_PUSH_WARNING(NO_IGNORE_RETURN_VALUE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

inline path trim(const path& value) NOEXCEPT
{
    // Trailing slash may cause successful create_directories returning false.
    return system::trim_right_copy(value.string(), { "/", "\\", "\x20" });
}

bool is_directory(const path& directory) NOEXCEPT
{
    return !get_is_directory(directory);
}

code get_is_directory(const path& directory) NOEXCEPT
{
    code ec{};
    std::filesystem::is_directory(system::to_extended_path(directory), ec);
    return ec;
}

bool clear_directory(const path& directory) NOEXCEPT
{
    return !clear_directory_ex(directory);
}

code clear_directory_ex(const path& directory) NOEXCEPT
{
    code ec{};
    const auto path = system::to_extended_path(directory);
    std::filesystem::remove_all(path, ec);
    if (ec) return ec;
    std::filesystem::create_directories(path, ec);
    return ec;
}

bool create_directory(const path& directory) NOEXCEPT
{
    return !create_directory_ex(directory);
}

code create_directory_ex(const path& directory) NOEXCEPT
{
    code ec{};
    const auto path = system::to_extended_path(trim(directory));
    std::filesystem::create_directories(path, ec);
    return ec;
}

bool is_file(const path& filename) NOEXCEPT
{
    return !get_is_file(filename);
}

code get_is_file(const path& filename) NOEXCEPT
{
    code ec{};
    std::filesystem::is_regular_file(filename, ec);
    return ec;
}

bool create_file(const path& filename) NOEXCEPT
{
    return !create_file_ex(filename);
}

// TODO: get more error info.
code create_file_ex(const path& filename) NOEXCEPT
{
    system::ofstream file(filename);
    const auto good = file.good();
    file.close();
    return good ? error::success : error::create_file;
}

bool create_file(const path& to, const uint8_t* data, size_t size) NOEXCEPT
{
    return !create_file_ex(to, data, size);
}

// TODO: get more error info.
code create_file_ex(const path& to, const uint8_t* data, size_t size) NOEXCEPT
{
    // Binary mode on Windows ensures that \n not replaced with \r\n.
    system::ofstream file(to, std::ios_base::binary);
    if (!file.good()) return error::create_file;
    file.write(pointer_cast<const char>(data), size);
    if (!file.good()) return error::create_file;
    file.close();
    return file.good() ? error::success : error::create_file;
}

// directory|file
bool remove(const path& name) NOEXCEPT
{
    return !remove_ex(name);
}

// directory|file
// Error code is not set if file did not exist.
code remove_ex(const path& name) NOEXCEPT
{
    code ec{};
    std::filesystem::remove(system::to_extended_path(name), ec);
    return ec;
}

// directory|file
bool rename(const path& from, const path& to) NOEXCEPT
{
    return !rename_ex(from, to);
}

// directory|file
code rename_ex(const path& from, const path& to) NOEXCEPT
{
    code ec{};
    std::filesystem::rename(system::to_extended_path(from),
        system::to_extended_path(to), ec);
    return ec;
}

// file
bool copy(const path& from, const path& to) NOEXCEPT
{
    return !copy_ex(from, to);
}

// file
code copy_ex(const path& from, const path& to) NOEXCEPT
{
    code ec{};
    std::filesystem::copy_file(system::to_extended_path(from),
        system::to_extended_path(to), ec);
    return ec;
}

// directory
bool copy_directory(const path& from, const path& to) NOEXCEPT
{
    return !copy_directory_ex(from, to);
}

// directory
code copy_directory_ex(const path& from, const path& to) NOEXCEPT
{
    if (file::is_directory(to) || !file::is_directory(from))
        return error::copy_directory;

    code ec{};
    std::filesystem::copy(system::to_extended_path(from),
        system::to_extended_path(to), ec);
    return ec;
}

// File descriptor functions required for memory mapping.

int open(const path& filename) NOEXCEPT
{
    const auto path = system::to_extended_path(filename);
    int file_descriptor{};

    // _wsopen_s and open set errno on failure.
    // _wsopen_s and wstring do not throw (but are unannotated).
#if defined(HAVE_MSC)
    // sets file_descriptor = -1 on error.
    _wsopen_s(&file_descriptor, path.c_str(),
        O_RDWR | _O_BINARY | _O_RANDOM, _SH_DENYWR, _S_IREAD | _S_IWRITE);
#else
    // returns -1 on failure.
    file_descriptor = ::open(path.c_str(),
        O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
    return file_descriptor;
}

code open_ex(int& file_descriptor, const path& filename) NOEXCEPT
{
    // TODO: map errno.
    errno = 0;
    file_descriptor = open(filename);
    return file_descriptor == -1 ? error::open_file : error::success;
}

bool close(int file_descriptor) NOEXCEPT
{
    // close and _close() set errno on failure.
#if defined(HAVE_MSC)
    return is_zero(::_close(file_descriptor));
#else
    return is_zero(::close(file_descriptor));
#endif
}

code close_ex(int file_descriptor) NOEXCEPT
{
    // TODO: map errno.
    errno = 0;
    return close(file_descriptor) ? error::success : error::close_file;
}

bool size(size_t& out, int file_descriptor) NOEXCEPT
{
    if (file_descriptor == -1)
    {
        errno = EINVAL;
        return false;
    }

    // _fstat64, _fstat32 and fstat set errno on failure.
    // This is required because off_t is defined as long, which is 32|64 bits
    // in msvc and 64 bits in linux/osx, and stat contains off_t.
#if defined(HAVE_MSC)
#if defined(HAVE_X64)
    struct _stat64 sbuf{};
    if (_fstat64(file_descriptor, &sbuf) == -1)
        return false;
#else
    struct _stat32 sbuf{};
    if (_fstat32(file_descriptor, &sbuf) == -1)
        return false;
#endif
#else
    // Limited to 32 bit files on 32 bit systems.
    struct stat sbuf{};
    if (fstat(file_descriptor, &sbuf) == -1)
        return false;
#endif
    if (is_limited<size_t>(sbuf.st_size))
    {
        errno = EOVERFLOW;
        return false;
    }

    out = sign_cast<size_t>(sbuf.st_size);
    return true;
}

code size_ex(size_t& out, int file_descriptor) NOEXCEPT
{
    // TODO: map errno.
    errno = 0;
    return size(out, file_descriptor) ? error::success : error::size_file;
}

bool size(size_t& out, const std::filesystem::path& filename) NOEXCEPT
{
    return !size_ex(out, filename);
}

code size_ex(size_t& out, const std::filesystem::path& filename) NOEXCEPT
{
    code ec{};
    const auto size = std::filesystem::file_size(to_extended_path(filename), ec);
    if (ec) return ec;
    if (is_limited<size_t>(size)) return error::size_overflow;
    out = possible_narrow_cast<size_t>(size);
    return ec;
}

bool space(size_t& out, const path& filename) NOEXCEPT
{
    return !space_ex(out, filename);
}

code space_ex(size_t& out, const path& filename) NOEXCEPT
{
    code ec{};
    const auto space = std::filesystem::space(to_extended_path(filename), ec);
    if (ec) return ec;
    if (is_limited<size_t>(space.available)) return error::space_overflow;
    out = possible_narrow_cast<size_t>(space.available);
    return ec;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace file
} // namespace database
} // namespace libbitcoin
