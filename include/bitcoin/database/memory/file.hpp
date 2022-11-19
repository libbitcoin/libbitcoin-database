/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_FILE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_FILE_HPP

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace file {

constexpr auto invalid = -1;
using path = std::filesystem::path;

/// Basic file system utilities, not thread safe.
BCD_API bool clear(const path& directory) NOEXCEPT;
BCD_API bool create(const path& filename) NOEXCEPT;
BCD_API bool copy(const path& to, const uint8_t* data, size_t size) NOEXCEPT;
BCD_API bool exists(const path& filename) NOEXCEPT;
BCD_API bool remove(const path& filename) NOEXCEPT;
BCD_API bool rename(const path& from, const path& to) NOEXCEPT;
BCD_API int open(const path& filename) NOEXCEPT;
BCD_API bool close(int file_descriptor) NOEXCEPT;
BCD_API size_t size(int file_descriptor) NOEXCEPT;
BCD_API size_t page() NOEXCEPT;


} // namespace file
} // namespace database
} // namespace libbitcoin

#endif
