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
#include "test.hpp"

namespace std {

std::ostream& operator<<(std::ostream& stream,
    const system::data_slice& slice) NOEXCEPT
{
    // Avoid serialize() here for its own test benefit.
    // stream << serialize(slice);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    stream << encode_base16(slice);
    BC_POP_WARNING()
    return stream;
}

} // namespace std

namespace test {

const std::string directory = "tests";

size_t size(const std::filesystem::path& file_path) NOEXCEPT
{
    // returns max_size_t on error.
    code ec;
    return system::possible_narrow_and_sign_cast<size_t>(
        std::filesystem::file_size(system::to_extended_path(file_path), ec));
}

bool exists(const std::filesystem::path& file_path) NOEXCEPT
{
    // Returns true only if file existed.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ifstream file(system::to_extended_path(file_path));
    const auto good = file.good();
    file.close();
    BC_POP_WARNING()
    return good;
}

bool remove(const std::filesystem::path& file_path) NOEXCEPT
{
    // Deletes and returns false if file did not exist (or error).
    code ec;
    return std::filesystem::remove(system::to_extended_path(file_path), ec);
}

bool clear(const std::filesystem::path& directory) NOEXCEPT
{
    // remove_all returns count removed, and error code if fails.
    // create_directories returns true if path exists or created.
    // used for setup, with no expectations of file/directory existence.
    const auto path = system::to_extended_path(directory);
    code ec;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::filesystem::remove_all(path, ec);
    return !ec && std::filesystem::create_directories(path, ec);
    BC_POP_WARNING()
}

bool folder(const std::filesystem::path& directory) NOEXCEPT
{
    const auto path = system::to_extended_path(directory);
    code ec;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return !ec && std::filesystem::is_directory(path, ec);
    BC_POP_WARNING()
}

bool create(const std::filesystem::path& file_path) NOEXCEPT
{
    // Creates and returns true if file existed or not (and no error).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ofstream file(system::to_extended_path(file_path));
    const auto good = file.good();
    file.close();
    BC_POP_WARNING()
    return good;
}

bool create(const std::filesystem::path& file_path,
    const std::string& text) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    system::ofstream file(file_path);
    const auto result = file.good();
    file << text;
    file.close();
    BC_POP_WARNING()
    return result;
}

std::string read_line(const std::filesystem::path& file_path,
    size_t line) NOEXCEPT
{
    std::string out{};
    std::ifstream file(system::to_extended_path(file_path));
    do { out.clear(); std::getline(file, out); } while (is_nonzero(line--));
    return out;
}

} // namespace test