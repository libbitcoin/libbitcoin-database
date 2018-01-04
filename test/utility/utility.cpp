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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <boost/filesystem.hpp>
#include <boost/functional/hash_fwd.hpp>
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

data_chunk generate_random_bytes(std::default_random_engine& engine,
    size_t size)
{
    data_chunk result(size);
    for (uint8_t& byte: result)
        byte = engine() % std::numeric_limits<uint8_t>::max();

    return result;
}

void create_database_file(const std::string& directory, size_t buckets,
    size_t total_txs, size_t tx_size)
{
    const auto header_size = slab_hash_table_header_size(buckets);

    test::create(directory);
    file_map file(directory);
    BOOST_REQUIRE(file.open());
    BOOST_REQUIRE(file.access()->buffer() != nullptr);
    file.resize(header_size + minimum_slabs_size);

    slab_hash_table_header header(file, buckets);
    BOOST_REQUIRE(header.create());
    BOOST_REQUIRE(header.start());

    const file_offset slab_start = header_size;

    slab_manager maanager(file, slab_start);
    BOOST_REQUIRE(maanager.create());
    BOOST_REQUIRE(maanager.start());

    slab_hash_table<hash_digest> table(header, maanager);

    std::default_random_engine engine;
    for (size_t i = 0; i < total_txs; ++i)
    {
        const auto value = generate_random_bytes(engine, tx_size);
        const auto key = bitcoin_hash(value);
        const auto write = [&value](byte_serializer& serial)
        {
            serial.write_forward(value);
        };

        table.store(key, write, value.size());
    }

    maanager.sync();
}

} // namespace test
