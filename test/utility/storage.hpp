/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
class storage
  : public bc::database::storage
{
public:
    storage();
    storage(bc::system::data_chunk&& initial);
    storage(const bc::system::data_chunk& initial);
    ~storage();

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
    bc::system::data_chunk buffer_;
    mutable bc::system::upgrade_mutex mutex_;
};

}

#endif
