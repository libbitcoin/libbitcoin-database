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
#include <bitcoin/database/databases/stealth_database.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;

constexpr size_t height_size = sizeof(uint32_t);
constexpr size_t prefix_size = sizeof(uint32_t);

// ephemkey is without sign byte and address is without version byte.
// [ prefix_bitfield:4 ][ height:32 ][ ephemkey:32 ][ address:20 ][ tx_id:32 ]
constexpr size_t row_size = prefix_size + height_size + hash_size +
    short_hash_size + hash_size;

stealth_database::stealth_database(const path& rows_filename,
    std::shared_ptr<shared_mutex> mutex)
  : rows_file_(rows_filename, mutex),
    rows_manager_(rows_file_, 0, row_size)
{
}

void stealth_database::create()
{
    rows_file_.resize(minimum_records_size);
    rows_manager_.create();
}

void stealth_database::start()
{
    rows_manager_.start();
}

bool stealth_database::stop()
{
    return rows_file_.stop();
}

// The prefix is fixed at 32 bits, but the filter is 0-32 bits, so the records
// cannot be indexed using a hash table. We also do not index by height.
stealth stealth_database::scan(const binary& filter, size_t from_height) const
{
    stealth result;

    for (auto row = 0; row < rows_manager_.count(); ++row)
    {
        const auto memory = rows_manager_.get(row);
        auto record = REMAP_ADDRESS(memory);

        // Skip if prefix doesn't match.
        const auto field = from_little_endian_unsafe<uint32_t>(record);
        if (!filter.is_prefix_of(field))
            continue;

        // Skip if height is too low.
        record += prefix_size;
        const auto height = from_little_endian_unsafe<uint32_t>(record);
        if (height < from_height)
            continue;

        // Add row to results.
        auto deserial = make_deserializer_unsafe(record + height_size);
        result.push_back(
        {
            deserial.read_hash(),
            deserial.read_short_hash(),
            deserial.read_hash()
        });
    }

    // TODO: we could sort result here.
    return result;
}

void stealth_database::store(uint32_t prefix, uint32_t height,
    const stealth_row& row)
{
    // Allocate new row.
    const auto index = rows_manager_.new_records(1);
    const auto memory = rows_manager_.get(index);
    const auto data = REMAP_ADDRESS(memory);

    // Write data.
    auto serial = make_serializer(data);

    // Dual key.
    serial.write_4_bytes_little_endian(prefix);
    serial.write_4_bytes_little_endian(height);

    // Stealth data.
    serial.write_hash(row.ephemeral_key);
    serial.write_short_hash(row.address);
    serial.write_hash(row.transaction_hash);
}

void stealth_database::unlink(size_t /* from_height */)
{
    // TODO: scan by height and mark as deleted.
}

void stealth_database::sync()
{
    rows_manager_.sync();
}

} // namespace database
} // namespace libbitcoin
