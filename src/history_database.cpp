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
#include <bitcoin/database.hpp>

#include <cstdint>
#include <cstddef>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/hash_table/record_multimap_iterator.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;

BC_CONSTEXPR size_t number_buckets = 97210744;
BC_CONSTEXPR size_t header_size = record_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_lookup_file_size = header_size + minimum_records_size;

BC_CONSTEXPR size_t record_size = hash_table_multimap_record_size<short_hash>();

BC_CONSTEXPR size_t value_size = 1 + 36 + 4 + 8;
BC_CONSTEXPR size_t row_record_size = hash_table_record_size<hash_digest>(value_size);

history_database::history_database(const path& lookup_filename,
    const path& rows_filename)
  : lookup_file_(lookup_filename), 
    header_(lookup_file_, number_buckets),
    manager_(lookup_file_, header_size, record_size),
    start_lookup_(header_, manager_),
    rows_file_(rows_filename), 
    rows_(rows_file_, 0, row_record_size),
    records_(rows_),
    map_(start_lookup_, records_)
{
    BITCOIN_ASSERT(REMAP_ADDRESS(lookup_file_.access()) != nullptr);
    BITCOIN_ASSERT(REMAP_ADDRESS(rows_file_.access()) != nullptr);
}

void history_database::create()
{
    lookup_file_.resize(initial_lookup_file_size);
    header_.create();
    manager_.create();

    rows_file_.resize(minimum_records_size);
    rows_.create();
}

void history_database::start()
{
    header_.start();
    manager_.start();
    rows_.start();
}

bool history_database::stop()
{
    return lookup_file_.stop() && rows_file_.stop();
}

void history_database::add_output(const short_hash& key,
    const output_point& outpoint, uint32_t output_height, uint64_t value)
{
    auto write = [&](uint8_t* data)
    {
        auto serial = make_serializer(data);
        serial.write_byte(0);
        auto raw_outpoint = outpoint.to_data();
        serial.write_data(raw_outpoint);
        serial.write_4_bytes_little_endian(output_height);
        serial.write_8_bytes_little_endian(value);
    };
    map_.add_row(key, write);
}

void history_database::add_spend(const short_hash& key,
    const output_point& previous, const input_point& spend,
    size_t spend_height)
{
    auto write = [&](uint8_t* data)
    {
        auto serial = make_serializer(data);
        serial.write_byte(1);
        auto raw_spend = spend.to_data();
        serial.write_data(raw_spend);
        serial.write_4_bytes_little_endian(spend_height);
        serial.write_8_bytes_little_endian(previous.checksum());
    };
    map_.add_row(key, write);
}

void history_database::delete_last_row(const short_hash& key)
{
    map_.delete_last_row(key);
}

// Each row contains a start byte which signals output or a spend.
inline point_kind marker_to_kind(uint8_t marker)
{
    BITCOIN_ASSERT(marker == 0 || marker == 1);
    return marker == 0 ? point_kind::output : point_kind::spend;
}

history history_database::get(const short_hash& key, size_t limit,
    size_t from_height) const
{
    // Read the height value from the row.
    const auto read_height = [](const uint8_t* data)
    {
        static constexpr file_offset height_position = 1 + 36;
        return from_little_endian_unsafe<uint32_t>(data + height_position);
    };

    // Read a row from the data for the history list.
    const auto read_row = [](const uint8_t* data)
    {
        auto deserial = make_deserializer_unsafe(data);
        return history_row
        {
            // output or spend?
            marker_to_kind(deserial.read_byte()),

            // point
            point::factory_from_data(deserial),

            // height
            deserial.read_4_bytes_little_endian(),

            // value or checksum
            { deserial.read_8_bytes_little_endian() }
        };
    };

    // This result is defined in libbitcoin.
    history result;
    const auto start = map_.lookup(key);
    for (const auto index: record_multimap_iterable(records_, start))
    {
        // Stop once we reach the limit (if specified).
        if (limit && result.size() >= limit)
            break;

        const auto memory = records_.get(index);
        const auto data = REMAP_ADDRESS(memory);

        // Skip rows below from_height (if specified).
        if (from_height && read_height(data) < from_height)
            continue;

        // Read this row into the list.
        result.emplace_back(read_row(data));
    }

    return result;
}

void history_database::sync()
{
    manager_.sync();
    rows_.sync();
}

history_statinfo history_database::statinfo() const
{
    return
    {
        header_.size(),
        manager_.count(),
        rows_.count()
    };
}

} // namespace database
} // namespace libbitcoin
