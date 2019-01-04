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
#include "utility.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <boost/filesystem.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;
using namespace bc::system;
using namespace boost::filesystem;
using namespace boost::system;

namespace test {

bool create(const path& file_path)
{
    system::ofstream file(file_path.string());

    if (!file.good())
        return false;

    file.put('z');
    return true;
}

bool exists(const path& file_path)
{
    system::ifstream file(file_path.string());
    return file.good();
}


void clear_path(const path& directory)
{
    error_code ec;
    log::initialize();
    remove_all(directory, ec);
    create_directories(directory, ec);
}

bool remove(const path& file_path)
{
    error_code ec;
    return boost::filesystem::remove(file_path, ec);
}

data_chunk generate_random_bytes(std::default_random_engine& engine,
    size_t size)
{
    data_chunk result(size);
    for (uint8_t& byte: result)
        byte = engine() % std::numeric_limits<uint8_t>::max();

    return result;
}

} // namespace test
