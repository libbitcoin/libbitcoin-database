/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/locks/file_lock.hpp>

#include <filesystem>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// locks, make_shared
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

file_lock::file_lock(const std::filesystem::path& file) NOEXCEPT
  : file_(file)
{
}

const std::filesystem::path& file_lock::file() const NOEXCEPT
{
    return file_;
}

bool file_lock::exists() const NOEXCEPT
{
    system::ifstream stream(file_);
    return stream.good();
}

// This is non-const as it alters state (externally but may become internal).
bool file_lock::create() NOEXCEPT
{
    system::ofstream stream(file_);
    return stream.good();
}

// This is non-const as it alters state (externally but may become internal).
bool file_lock::destroy() NOEXCEPT
{
    // remove returns false if file did not exist though error_code is false if
    // file did not exist. use of error_code overload also precludes exception.
    std::error_code ec;
    std::filesystem::remove(system::extended_path(file_), ec);
    return !ec;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
