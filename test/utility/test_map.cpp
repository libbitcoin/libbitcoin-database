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
#include "test_map.hpp"

#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;

namespace test {

test_map::test_map()
{
}

bool test_map::open()
{
    return false;
}

bool test_map::flush() const
{
    return false;
}

bool test_map::close()
{
    return false;
}

bool test_map::closed() const
{
    return false;
}

size_t test_map::size() const
{
    return 0;
}

memory_ptr test_map::access()
{
    return nullptr;
}

memory_ptr test_map::resize(size_t size)
{
    return nullptr;
}

memory_ptr test_map::reserve(size_t size)
{
    return nullptr;
}

} // namespace test
