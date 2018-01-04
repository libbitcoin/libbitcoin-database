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
#ifndef TEST_MAP_HPP
#define TEST_MAP_HPP

#include <bitcoin/database.hpp>

namespace test {

// Fake a thread safe memory map implementation.
class test_map
  : public bc::database::memory_map
{
public:
    test_map();
    test_map(const bc::data_chunk& initial);
    ~test_map();

    bool open();
    bool flush() const;
    bool close();
    bool closed() const;
    size_t size() const;
    bc::database::memory_ptr access();
    bc::database::memory_ptr resize(size_t size);
    bc::database::memory_ptr reserve(size_t size);

private:
    bool closed_;
    bc::data_chunk buffer_;
    mutable bc::upgrade_mutex mutex_;
};

}

#endif
