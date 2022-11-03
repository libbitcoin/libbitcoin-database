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

/// Basic file system utilities, not thread safe.
BCD_API bool clear(const std::filesystem::path& directory) NOEXCEPT;
BCD_API bool create(const std::filesystem::path& filename) NOEXCEPT;
BCD_API bool exists(const std::filesystem::path& filename) NOEXCEPT;
BCD_API bool remove(const std::filesystem::path& filename) NOEXCEPT;
BCD_API bool rename(const std::filesystem::path& from,
	const std::filesystem::path& to) NOEXCEPT;
BCD_API int open(const std::filesystem::path& filename) NOEXCEPT;
BCD_API bool close(int file_descriptor) NOEXCEPT;
BCD_API size_t size(int file_descriptor) NOEXCEPT;
BCD_API size_t page() NOEXCEPT;

} // namespace file
} // namespace database
} // namespace libbitcoin

#endif