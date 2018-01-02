/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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

#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;
using namespace boost::filesystem;
using namespace boost::system;

namespace test {

bool create(const path& file_path)
{
    bc::ofstream file(file_path.string());

    if (file.bad())
        return false;

    file.put('z');
    return true;
}

bool exists(const path& file_path)
{
    bc::ifstream file(file_path.string());
    return file.good();
}


bool clear_path(const boost::filesystem::path& directory)
{
    error_code ec;
    log::initialize();
    remove_all(directory, ec);
    return create_directories(directory, ec);
}

} // namespace test
