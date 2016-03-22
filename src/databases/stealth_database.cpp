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

// ephemkey is without sign byte and address is without version byte.
constexpr size_t prefix_size = sizeof(uint32_t);

// [ prefix_bitfield:4 ][ ephemkey:32 ][ address:20 ][ tx_id:32 ]
constexpr size_t row_size = prefix_size + 2 * hash_size + short_hash_size;

stealth_database::stealth_database(const path& index_filename,
    const path& rows_filename, std::shared_ptr<shared_mutex> mutex)
  : index_file_(index_filename, mutex),
    index_manager_(index_file_, 0, sizeof(array_index)),
    rows_file_(rows_filename, mutex),
    rows_manager_(rows_file_, 0, row_size)
{
}

void stealth_database::create()
{
    index_file_.resize(minimum_records_size);
    index_manager_.create();
    rows_file_.resize(minimum_records_size);
    rows_manager_.create();
}

void stealth_database::start()
{
    index_manager_.start();
    rows_manager_.start();
    row_count_ = rows_manager_.count();
}

bool stealth_database::stop()
{
    return index_file_.stop() && rows_file_.stop();
}

// Stealth records are not indexed. The prefix is fixed at 32 bits, but the
// filter is 0-32 bits, so the records cannot be indexed using a hash table.
stealth stealth_database::scan(const binary& filter, size_t from_height) const
{
    stealth result;

    if (from_height >= index_manager_.count())
        return result;

    const auto start = read_index(from_height);

    for (auto index = start; index < rows_manager_.count(); ++index)
    {
        // see if prefix matches
        const auto memory = rows_manager_.get(index);
        const auto record = REMAP_ADDRESS(memory);
        const auto field = from_little_endian_unsafe<uint32_t>(record);
        if (!filter.is_prefix_of(field))
            continue;

        // Add row to results.
        auto deserial = make_deserializer_unsafe(record + prefix_size);
        result.push_back(
        {
            deserial.read_hash(),
            deserial.read_short_hash(),
            deserial.read_hash()
        });
    }

    return result;
}

void stealth_database::store(uint32_t prefix, const stealth_row& row)
{
    // Allocate new row.
    const auto index = rows_manager_.new_records(1);
    const auto memory = rows_manager_.get(index);
    const auto data = REMAP_ADDRESS(memory);

    // Write data.
    auto serial = make_serializer(data);
    serial.write_4_bytes_little_endian(prefix);
    serial.write_hash(row.ephemeral_key);
    serial.write_short_hash(row.address);
    serial.write_hash(row.transaction_hash);

    write_index();

    BITCOIN_ASSERT(serial.iterator() == data + prefix_size + hash_size +
        short_hash_size + hash_size);
}

void stealth_database::unlink(size_t from_height)
{
    if (index_manager_.count() > from_height)
        index_manager_.set_count(from_height);
}

void stealth_database::sync()
{
    rows_manager_.sync();
    index_manager_.sync();
}

// Read/write of this value protected by sync.
void stealth_database::write_index()
{
    const auto index = index_manager_.new_records(1);
    const auto memory = index_manager_.get(index);
    auto serial = make_serializer(REMAP_ADDRESS(memory));
    serial.write_4_bytes_little_endian(row_count_);
    row_count_ = rows_manager_.count();
}

array_index stealth_database::read_index(size_t from_height) const
{
    BITCOIN_ASSERT(from_height < index_manager_.count());
    const auto memory = index_manager_.get(from_height);
    const auto address = REMAP_ADDRESS(memory);
    return from_little_endian_unsafe<array_index>(address);
}

} // namespace database
} // namespace libbitcoin
