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
#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <cstddef>
#include <random>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <bitcoin/database.hpp>

#define TEST_NAME \
    std::string(boost::unit_test::framework::current_test_case().p_name)

namespace test {

typedef bc::system::byte_array<4> tiny_hash;
typedef bc::system::byte_array<8> little_hash;

bool create(const boost::filesystem::path& file_path);
bool exists(const boost::filesystem::path& file_path);
bool remove(const boost::filesystem::path& file_path);
void clear_path(const boost::filesystem::path& directory);

} // namspace test

namespace std
{

// Extend std namespace with tiny_hash wrapper.
template <>
struct hash<test::tiny_hash>
{
    size_t operator()(const test::tiny_hash& value) const
    {
        return boost::hash_range(value.begin(), value.end());
    }
};

// Extend std namespace with little_hash wrapper.
template <>
struct hash<test::little_hash>
{
    size_t operator()(const test::little_hash& value) const
    {
        return boost::hash_range(value.begin(), value.end());
    }
};

} // namspace std


#endif
