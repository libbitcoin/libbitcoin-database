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
#ifndef LIBBITCOIN_DATABASE_FILE_UTILITIES_HPP
#define LIBBITCOIN_DATABASE_FILE_UTILITIES_HPP

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace file {

constexpr auto invalid = -1;
using path = std::filesystem::path;

/// True only if directory existed.
BCD_API bool is_directory(const path& directory) NOEXCEPT;

/// Clear and recreate directory, true if path existed/created.
BCD_API bool clear_directory(const path& directory) NOEXCEPT;

/// Create directory, false if existed/error.
BCD_API bool create_directory(const path& directory) NOEXCEPT;

/// True only if file existed.
BCD_API bool is_file(const path& filename) NOEXCEPT;

/// Create/open/close file or open/close if existed.
BCD_API bool create_file(const path& filename) NOEXCEPT;

/// Create/open file and initialize/replace with size/data.
BCD_API bool create_file(const path& to, const uint8_t* data,
    size_t size) NOEXCEPT;

/// Delete file or empty directory, false on error only.
BCD_API bool remove(const path& name) NOEXCEPT;

/// Rename file or directory, false if did not exist/error.
BCD_API bool rename(const path& from, const path& to) NOEXCEPT;

/// Copy file, false if did not exist/error or target existed.
BCD_API bool copy(const path& from, const path& to) NOEXCEPT;

/// Copy directory with contents non-recursively.
/// False if did not exist/error or target existed.
BCD_API bool copy_directory(const path& from, const path& to) NOEXCEPT;

/// File descriptor functions (for memory mapping).
BCD_API int open(const path& filename) NOEXCEPT;
BCD_API bool close(int file_descriptor) NOEXCEPT;
BCD_API bool size(size_t& out, int file_descriptor) NOEXCEPT;

/// File size from name.
BCD_API bool size(size_t& out, const path& filename) NOEXCEPT;

/// Volume space from path.
BCD_API bool space(size_t& out, const path& filename) NOEXCEPT;

} // namespace file
} // namespace database
} // namespace libbitcoin

#endif
