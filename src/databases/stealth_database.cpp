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
#include <bitcoin/database/databases/stealth_database.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

// Record format (v4):
// ----------------------------------------------------------------------------
// [ height:4    - const ] (first short-circuit sequential read after height)
// [ prefix:4    - const ] (second short-circuit sequential read after prefix)
// [ ephemkey:32 - const ]
// [ address:20  - const ]
// [ tx_hash:32  - const ]

// Record format (v3):
// ----------------------------------------------------------------------------
// [ prefix:4    - const ]
// [ height:4    - const ]
// [ ephemkey:32 - const ]
// [ address:20  - const ]
// [ tx_hash:32  - const ]

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto prefix_size = sizeof(uint32_t);

// ephemkey is without sign byte and address is without version byte.
static constexpr auto value_size = prefix_size + height_size + hash_size +
    short_hash_size + hash_size;

// Stealth uses an unindexed array, requiring linear search, (O(n)).
stealth_database::stealth_database(const path& rows_filename, size_t expansion)
  : stealth_file_(rows_filename, expansion),
    stealth_index_(stealth_file_, 0, value_size)
{
}

stealth_database::~stealth_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool stealth_database::create()
{
    if (!stealth_file_.open())
        return false;

    // No need to call open after create.
    return
        stealth_index_.create();
}

bool stealth_database::open()
{
    return
        stealth_file_.open() &&
        stealth_index_.start();
}

void stealth_database::commit()
{
    stealth_index_.sync();
}

bool stealth_database::flush() const
{
    return stealth_file_.flush();
}

bool stealth_database::close()
{
    return stealth_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

// The prefix is fixed at 32 bits, but the filter is 0-32 bits, so the records
// cannot be indexed using a hash table, and are not indexed by height.
stealth_database::list stealth_database::get(const binary& filter,
    size_t from_height) const
{
    list result;
    stealth_record stealth;

    for (array_index row = 0; row < stealth_index_.count(); ++row)
    {
        const auto record = stealth_index_.get(row);
        auto deserial = make_unsafe_deserializer(record->buffer());

        // Failed reads are conflated with skipped returns.
        if (stealth.from_data(deserial, from_height, filter))
            result.push_back(stealth);
    }

    return result;
}

// Store.
// ----------------------------------------------------------------------------

void stealth_database::store(const stealth_record& stealth)
{
    // Allocate new row and write data.
    const auto index = stealth_index_.new_records(1);
    const auto record = stealth_index_.get(index);
    const auto memory = record->buffer();
    auto serial = make_unsafe_serializer(memory);
    stealth.to_data(serial, false);
}

} // namespace database
} // namespace libbitcoin
