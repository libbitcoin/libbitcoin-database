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
#include <bitcoin/database/transaction_database.hpp>

#include <cstddef>
#include <cstdint>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

constexpr size_t number_buckets = 100000000;
BC_CONSTEXPR size_t header_size = htdb_slab_header_fsize(number_buckets);
BC_CONSTEXPR size_t initial_map_file_size = header_size + min_slab_fsize;

BC_CONSTEXPR file_offset allocation_offset = header_size;

transaction_database::transaction_database(
    const boost::filesystem::path& map_filename)
  : map_file_(map_filename), 
    header_(map_file_, 0),
    manager_(map_file_, allocation_offset),
    map_(header_, manager_)
{
    BITCOIN_ASSERT(map_file_.access()->buffer() != nullptr);
}

void transaction_database::create()
{
    map_file_.resize(initial_map_file_size);
    header_.create(number_buckets);
    manager_.create();
}

void transaction_database::start()
{
    header_.start();
    manager_.start();
}

bool transaction_database::stop()
{
    return map_file_.stop();
}

transaction_result transaction_database::get(const hash_digest& hash) const
{
    const auto slab = map_.get2(hash);
    return transaction_result(slab);
}

void transaction_database::store(size_t height, size_t index,
    const chain::transaction& tx)
{
    // Write block data.
    const hash_digest key = tx.hash();
    const size_t value_size = 4 + 4 + tx.serialized_size();
    auto write = [&height, &index, &tx](uint8_t* data)
    {
        auto serial = make_serializer(data);
        serial.write_4_bytes_little_endian(height);
        serial.write_4_bytes_little_endian(index);
        const auto tx_data = tx.to_data();
        serial.write_data(tx_data);
    };
    map_.store(key, write, value_size);
}

void transaction_database::remove(const hash_digest& hash)
{
    DEBUG_ONLY(bool success =) map_.unlink(hash);
    BITCOIN_ASSERT(success);
}

void transaction_database::sync()
{
    manager_.sync();
}

} // namespace database
} // namespace libbitcoin
