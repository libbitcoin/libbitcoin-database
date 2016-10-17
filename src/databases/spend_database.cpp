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
#include <bitcoin/database/databases/spend_database.hpp>

#include <algorithm>
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

BC_CONSTEXPR size_t number_buckets = 228110589;
BC_CONSTEXPR size_t header_size = record_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_map_file_size = header_size + minimum_records_size;

BC_CONSTEXPR size_t value_size = std::tuple_size<chain::point>::value;
BC_CONSTEXPR size_t record_size = hash_table_record_size<chain::point>(value_size);

spend_database::spend_database(const path& filename,
    std::shared_ptr<shared_mutex> mutex)
  : lookup_file_(filename, mutex), 
    lookup_header_(lookup_file_, number_buckets),
    lookup_manager_(lookup_file_, header_size, record_size),
    lookup_map_(lookup_header_, lookup_manager_)
{
}

// Close does not call stop because there is no way to detect thread join.
spend_database::~spend_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool spend_database::create()
{
    // Resize and create require a started file.
    if (!lookup_file_.open())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool spend_database::open()
{
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

bool spend_database::close()
{
    return lookup_file_.close();
}

// ----------------------------------------------------------------------------

input_point spend_database::get(const output_point& outpoint) const
{
    input_point point;
    const auto memory = lookup_map_.find(outpoint);

    if (!memory)
        return point;

    // The order of properties in this serialization was changed in v3.
    // Previously it was { index, hash }, which was inconsistent with wire.
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(memory));
    point.from_data(deserial);
    return point;
}

void spend_database::store(const chain::output_point& outpoint,
    const chain::input_point& spend)
{
    const auto write = [&spend](memory_ptr data)
    {
        auto serial = make_unsafe_serializer(REMAP_ADDRESS(data));
        serial.write_bytes(spend.to_data());
    };

    lookup_map_.store(outpoint, write);
}

bool spend_database::unlink(const output_point& outpoint)
{
    return lookup_map_.unlink(outpoint);
}

void spend_database::sync()
{
    lookup_manager_.sync();
}

spend_statinfo spend_database::statinfo() const
{
    return
    {
        lookup_header_.size(),
        lookup_manager_.count()
    };
}

} // namespace database
} // namespace libbitcoin
