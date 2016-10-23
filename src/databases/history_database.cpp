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
#include <bitcoin/database/databases/history_database.hpp>

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_multimap_iterable.hpp>
#include <bitcoin/database/primitives/record_multimap_iterator.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr auto rows_header_size = 0u;

static constexpr auto flag_size = sizeof(uint8_t);
static constexpr auto point_size = hash_size + sizeof(uint32_t);
static constexpr auto height_position = flag_size + point_size;
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto checksum_size = sizeof(uint64_t);
static constexpr auto value_size = flag_size + point_size + height_size +
    checksum_size;

static BC_CONSTEXPR auto record_size = 
    hash_table_multimap_record_size<short_hash>();
static BC_CONSTEXPR auto row_record_size = 
    hash_table_record_size<hash_digest>(value_size);

// History uses a hash table index, O(1).
history_database::history_database(const path& lookup_filename,
    const path& rows_filename, size_t buckets, size_t expansion,
    mutex_ptr mutex)
  : initial_map_file_size_(record_hash_table_header_size(buckets) +
        minimum_records_size),

    lookup_file_(lookup_filename, mutex, expansion), 
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, record_hash_table_header_size(buckets),
        record_size),
    lookup_map_(lookup_header_, lookup_manager_),

    rows_file_(rows_filename, mutex, expansion),
    rows_manager_(rows_file_, rows_header_size, row_record_size),
    rows_list_(rows_manager_),
    rows_multimap_(lookup_map_, rows_list_)
{
}

history_database::~history_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool history_database::create()
{
    // Resize and create require an opened file.
    if (!lookup_file_.open() ||
        !rows_file_.open())
        return false;

    // These will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);
    rows_file_.resize(minimum_records_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create() ||
        !rows_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start() &&
        rows_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool history_database::open()
{
    return
        lookup_file_.open() &&
        rows_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        rows_manager_.start();
}

bool history_database::close()
{
    return
        lookup_file_.close() &&
        rows_file_.close();
}

// Commit latest inserts.
void history_database::synchronize()
{
    lookup_manager_.sync();
    rows_manager_.sync();
}

// Flush the memory maps to disk.
bool history_database::flush()
{
    return
        lookup_file_.flush() &&
        rows_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

void history_database::add_output(const short_hash& key,
    const output_point& outpoint, uint32_t output_height, uint64_t value)
{
    // TODO: use output_point serialization.
    auto write = [&](memory_ptr data)
    {
        auto serial = make_unsafe_serializer(REMAP_ADDRESS(data));
        serial.write_byte(static_cast<uint8_t>(point_kind::output));
        serial.write_bytes(outpoint.to_data());
        serial.write_4_bytes_little_endian(output_height);
        serial.write_8_bytes_little_endian(value);
    };
    rows_multimap_.add_row(key, write);
}

void history_database::add_input(const short_hash& key,
    const output_point& inpoint, uint32_t input_height,
    const input_point& previous)
{
    // TODO: use input_point serialization.
    auto write = [&](memory_ptr data)
    {
        auto serial = make_unsafe_serializer(REMAP_ADDRESS(data));
        serial.write_byte(static_cast<uint8_t>(point_kind::spend));
        serial.write_bytes(inpoint.to_data());
        serial.write_4_bytes_little_endian(input_height);
        serial.write_8_bytes_little_endian(previous.checksum());
    };
    rows_multimap_.add_row(key, write);
}

// This is the history unlink.
bool history_database::delete_last_row(const short_hash& key)
{
    return rows_multimap_.delete_last_row(key);
}

history_compact::list history_database::get(const short_hash& key,
    size_t limit, size_t from_height) const
{
    // Read the height value from the row.
    const auto read_height = [](uint8_t* data)
    {
        return from_little_endian_unsafe<uint32_t>(data + height_position);
    };

    // TODO: add serialization to history_compact.
    // Read a row from the data for the history list.
    const auto read_row = [](uint8_t* data)
    {
        auto deserial = make_unsafe_deserializer(data);
        return history_compact
        {
            // output or spend?
            static_cast<point_kind>(deserial.read_byte()),

            // point
            point::factory_from_data(deserial),

            // height
            deserial.read_4_bytes_little_endian(),

            // value or checksum
            { deserial.read_8_bytes_little_endian() }
        };
    };

    history_compact::list result;
    const auto start = rows_multimap_.lookup(key);
    const auto records = record_multimap_iterable(rows_list_, start);

    for (const auto index: records)
    {
        // Stop once we reach the limit (if specified).
        if (limit > 0 && result.size() >= limit)
            break;

        // This obtains a remap safe address pointer against the rows file.
        const auto record = rows_list_.get(index);
        const auto address = REMAP_ADDRESS(record);

        // Skip rows below from_height.
        if (from_height == 0 || read_height(address) >= from_height)
            result.push_back(read_row(address));
    }

    // TODO: we could sort result here.
    return result;
}

history_statinfo history_database::statinfo() const
{
    return
    {
        lookup_header_.size(),
        lookup_manager_.count(),
        rows_manager_.count()
    };
}

} // namespace database
} // namespace libbitcoin
