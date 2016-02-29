/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/block_database.hpp>

#include <cstdint>
#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/result/block_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using path = boost::filesystem::path;

BC_CONSTEXPR size_t number_buckets = 600000;
BC_CONSTEXPR size_t header_size = slab_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_map_file_size = header_size + minimum_slabs_size;

BC_CONSTEXPR file_offset allocation_offset = header_size;

// Record format:
// main:
//  [ header:80      ]
//  [ height:4       ]
//  [ number_txs:4   ]
// hashes:
//  [ [    ...     ] ]
//  [ [ tx_hash:32 ] ]
//  [ [    ...     ] ]

block_database::block_database(const path& map_filename,
    const path& index_filename)
  : map_file_(map_filename), 
    header_(map_file_, 0),
    manager_(map_file_, allocation_offset),
    map_(header_, manager_),
    index_file_(index_filename),
    index_(index_file_, 0, sizeof(file_offset))
{
    BITCOIN_ASSERT(map_file_.access()->buffer() != nullptr);
    BITCOIN_ASSERT(index_file_.access()->buffer() != nullptr);
}

void block_database::create()
{
    map_file_.resize(initial_map_file_size);
    header_.create(number_buckets);
    manager_.create();
    index_file_.resize(minimum_records_size);
    index_.create();
}

void block_database::start()
{
    header_.start();
    manager_.start();
    index_.start();
}

bool block_database::stop()
{
    return map_file_.stop() && index_file_.stop();
}

block_result block_database::get(const size_t height) const
{
    if (height >= index_.count())
        return block_result(nullptr);

    const auto position = read_position(height);
    const auto memory = manager_.get(position);
    return block_result(memory->buffer());
}

block_result block_database::get(const hash_digest& hash) const
{
    const auto slab = map_.get2(hash);
    return block_result(slab);
}

void block_database::store(const chain::block& block)
{
    const uint32_t height = index_.count();
    const auto number_txs = block.transactions.size();
    const uint32_t number_txs32 = static_cast<uint32_t>(number_txs);

    // Write block data.
    const auto write = [&](uint8_t* data)
    {
        auto serial = make_serializer(data);
        data_chunk header_data = block.header.to_data(false);
        serial.write_data(header_data);
        serial.write_4_bytes_little_endian(height);
        serial.write_4_bytes_little_endian(number_txs32);

        for (const auto& tx: block.transactions)
        {
            const auto tx_hash = tx.hash();
            serial.write_hash(tx_hash);
        }
    };

    const auto key = block.header.hash();
    const auto value_size = 80 + 4 + 4 + number_txs * hash_size;
    const auto position = map_.store(key, write, value_size);

    // Write height -> position mapping.
    write_position(position);
}

void block_database::unlink(const size_t from_height)
{
    index_.set_count(from_height);
}

void block_database::sync()
{
    manager_.sync();
    index_.sync();
}

bool block_database::top(size_t& out_height) const
{
    if (index_.count() == 0)
        return false;

    out_height = index_.count() - 1;
    return true;
}

////size_t block_database::gap(size_t start) const
////{
////    for (int height = 0; height < max_size_t; height++)
////        const auto position = get(height);
////
////    return 0;
////}

void block_database::write_position(const file_offset position)
{
    const auto index = index_.new_record();
    const auto memory = index_.get(index);
    auto serial = make_serializer(memory->buffer());

    // MUST BE ATOMIC
    serial.write_8_bytes_little_endian(position);
}

file_offset block_database::read_position(const array_index index) const
{
    const auto record = index_.get(index);
    return from_little_endian_unsafe<file_offset>(record->buffer());
}

} // namespace database
} // namespace libbitcoin
