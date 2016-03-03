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
#include <bitcoin/database/stealth_database.hpp>

#include <cstddef>
#include <cstdint>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;

// ephemkey is without sign byte and address is without version byte.
constexpr size_t prefix_size = sizeof(uint32_t);

// [ prefix_bitfield:4 ][ ephemkey:32 ][ address:20 ][ tx_id:32 ]
constexpr size_t row_size = prefix_size + 2 * hash_size + short_hash_size;

stealth_database::stealth_database(const path& index_filename,
    const path& rows_filename)
  : index_file_(index_filename),
    index_(index_file_, 0, sizeof(array_index)),
    rows_file_(rows_filename),
    rows_(rows_file_, 0, row_size)
{
}

void stealth_database::create()
{
    index_file_.allocate(minimum_records_size);
    index_.create();
    rows_file_.allocate(minimum_records_size);
    rows_.create();
}

void stealth_database::start()
{
    index_.start();
    rows_.start();
    block_start_ = rows_.count();
}

bool stealth_database::stop()
{
    return index_file_.stop() && rows_file_.stop();
}

// Stealth records are not indexed. The prefix is fixed at 32 bits, but the
// filter is 0-32 bits, so the records cannot be indexed using a hash table.
stealth stealth_database::scan(const binary& filter, size_t from_height) const
{
    if (from_height >= index_.count())
        return stealth();

    // This result is defined in libbitcoin.
    stealth result;
    const auto start = read_index(from_height);
    for (auto index = start; index < rows_.count(); ++index)
    {
        // see if prefix matches
        const auto record = rows_.get(index);
        const auto field = from_little_endian_unsafe<uint32_t>(record->buffer());
        if (!filter.is_prefix_of(field))
            continue;

        // Add row to results.
        auto deserial = make_deserializer_unsafe(record->buffer() + prefix_size);
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
    const auto index = rows_.new_records(1);
    const auto memory = rows_.get(index);
    const auto data = memory->buffer();

    // Write data.
    auto serial = make_serializer(data);
    serial.write_4_bytes_little_endian(prefix);
    serial.write_hash(row.ephemeral_key);
    serial.write_short_hash(row.address);
    serial.write_hash(row.transaction_hash);

    BITCOIN_ASSERT(serial.iterator() == data + prefix_size + hash_size +
        short_hash_size + hash_size);
}

void stealth_database::unlink(size_t from_height)
{
    BITCOIN_ASSERT(index_.count() > from_height);
    index_.set_count(from_height);
}

void stealth_database::sync()
{
    rows_.sync();
    write_index();
}

void stealth_database::write_index()
{
    // Write index of first row into block lookup index.
    const auto index = index_.new_records(1);
    const auto memory = index_.get(index);
    auto serial = make_serializer(memory->buffer());

    // MUST BE ATOMIC
    serial.write_4_bytes_little_endian(block_start_);

    // Synchronise data.
    index_.sync();

    // Prepare for next block.
    block_start_ = rows_.count();
}

array_index stealth_database::read_index(size_t from_height) const
{
    BITCOIN_ASSERT(from_height < index_.count());
    const auto record = index_.get(from_height);
    return from_little_endian_unsafe<array_index>(record->buffer());
}

} // namespace database
} // namespace libbitcoin
