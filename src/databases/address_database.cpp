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
#include <bitcoin/database/databases/address_database.hpp>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_multimap.hpp>

// Record format (v4) [47 bytes]:
// ----------------------------------------------------------------------------
// [ height:4      - const] (may short-circuit sequential read after height)
// [ kind:1        - const]
// [ point-hash:32 - const]
// [ point-index:2 - const]
// [ data:8        - const]

// Record format (v3) [47 bytes]:
// ----------------------------------------------------------------------------
// [ kind:1        - const]
// [ point-hash:32 - const]
// [ point-index:2 - const]
// [ height:4      - const]
// [ data:8        - const]

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto flag_size = sizeof(uint8_t);
static constexpr auto point_size = std::tuple_size<point>::value;
static constexpr auto checksum_size = sizeof(uint64_t);
static constexpr auto value_size = height_size + flag_size + point_size +
    checksum_size;

// History uses a hash table index, O(1).
// The hash table stores indexes to the first element of unkeyed linked lists.
address_database::address_database(const path& lookup_filename,
    const path& rows_filename, size_t buckets, size_t expansion)
  : hash_table_file_(lookup_filename, expansion),

    // THIS sizeof(link_type) IS ASSUMED BY hash_table_multimap.
    hash_table_(hash_table_file_, buckets, sizeof(link_type)),

    address_file_(rows_filename, expansion),
    address_index_(address_file_, 0,
        hash_table_multimap<key_type, index_type, link_type>::size(value_size)),
    address_recordset_map_(hash_table_, address_index_)
{
}

address_database::~address_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool address_database::create()
{
    if (!hash_table_file_.open() ||
        !address_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create() &&
        address_index_.create();
}

bool address_database::open()
{
    return
        hash_table_file_.open() &&
        address_file_.open() &&
        hash_table_.start() &&
        address_index_.start();
}

void address_database::commit()
{
    hash_table_.commit();
    address_index_.commit();
}

bool address_database::flush() const
{
    return
        hash_table_file_.flush() &&
        address_file_.flush();
}

bool address_database::close()
{
    return
        hash_table_file_.close() &&
        address_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

address_database::list address_database::get(const short_hash& key,
    size_t limit, size_t from_height) const
{
    list result;
    payment_record payment;

    // Declare reusable reader.
    const auto reader = [&](byte_deserializer& deserial)
    {
        payment.from_data(deserial, from_height);
    };

    // Get an iterator for the set of records that matches the key.
    auto history = address_recordset_map_.find(key);

    for (const auto element: history)
    {
        if (limit > 0 && result.size() >= limit)
            break;

        element.read(reader);

        BITCOIN_ASSERT(payment.is_valid());
        result.push_back(std::move(payment));
    }

    return result;
}

// Store.
// ----------------------------------------------------------------------------

void address_database::store(const short_hash& key,
    const payment_record& payment)
{
    const auto writer = [&](byte_serializer& serial)
    {
        payment.to_data(serial, false);
    };

    // Write the new payment history.
    auto front = address_recordset_map_.allocator();
    front.create(writer);
    address_recordset_map_.link(key, front);
}

// Update.
// ----------------------------------------------------------------------------

bool address_database::unlink_last_row(const short_hash& key)
{
    return address_recordset_map_.unlink(key);
}

} // namespace database
} // namespace libbitcoin
