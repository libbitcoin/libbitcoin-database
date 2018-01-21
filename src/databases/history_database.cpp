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
#include <bitcoin/database/databases/history_database.hpp>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_multimap.hpp>
#include <bitcoin/database/primitives/record_list_iterable.hpp>

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
// The hash table stores indexes into the first element of a multimap row.
history_database::history_database(const path& lookup_filename,
    const path& rows_filename, size_t buckets, size_t expansion)
  : hash_table_file_(lookup_filename, expansion),
    hash_table_(hash_table_file_, buckets, sizeof(link_type)),

    address_file_(rows_filename, expansion),
    address_index_(address_file_, 0, record_multimap<key_type, index_type, link_type>::size(value_size)),
    address_multimap_(hash_table_, address_index_)
{
}

history_database::~history_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool history_database::create()
{
    if (!hash_table_file_.open() ||
        !address_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create() &&
        address_index_.create();
}

bool history_database::open()
{
    return
        hash_table_file_.open() &&
        address_file_.open() &&
        hash_table_.start() &&
        address_index_.start();
}

void history_database::commit()
{
    hash_table_.sync();
    address_index_.sync();
}

bool history_database::flush() const
{
    return
        hash_table_file_.flush() &&
        address_file_.flush();
}

bool history_database::close()
{
    return
        hash_table_file_.close() &&
        address_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

history_database::list history_database::get(const short_hash& key,
    size_t limit, size_t from_height) const
{
    list result;
    payment_record payment;
    const auto start = address_multimap_.find(key);

    // TODO: expose iterator from manager.
    auto records = record_list_iterable<link_type>(address_index_, start);

    for (const auto index: records)
    {
        if (limit > 0 && result.size() >= limit)
            break;

        const auto record = address_multimap_.get(index);
        auto deserial = make_unsafe_deserializer(record->buffer());

        // Failed reads are conflated with skipped returns.
        if (payment.from_data(deserial, from_height))
            result.push_back(payment);
    }

    return result;
}

// Store.
// ----------------------------------------------------------------------------

void history_database::store(const short_hash& key,
    const payment_record& payment)
{
    const auto write = [&](byte_serializer& serial)
    {
        payment.to_data(serial, false);
    };

    address_multimap_.store(key, write);
}

// Update.
// ----------------------------------------------------------------------------

bool history_database::unlink_last_row(const short_hash& key)
{
    return address_multimap_.unlink(key);
}

} // namespace database
} // namespace libbitcoin
