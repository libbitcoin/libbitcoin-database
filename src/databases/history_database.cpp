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
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_hash_table.hpp>
#include <bitcoin/database/primitives/record_multimap.hpp>
#include <bitcoin/database/primitives/record_multimap_iterable.hpp>
#include <bitcoin/database/primitives/record_row.hpp>

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
  : lookup_file_(lookup_filename, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_,
        record_map::header_type::size(buckets),
        record_row<short_hash>::size(sizeof(array_index))),
    lookup_map_(lookup_header_, lookup_manager_),

    rows_file_(rows_filename, expansion),
    rows_manager_(rows_file_, 0,
        record_multimap<short_hash>::element_size(value_size)),
    rows_multimap_(lookup_map_, rows_manager_)
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
    if (!lookup_file_.open() ||
        !rows_file_.open())
        return false;

    // No need to call open after create.
    return
        lookup_header_.create() &&
        lookup_manager_.create() &&
        rows_manager_.create();
}

bool history_database::open()
{
    return
        lookup_file_.open() &&
        rows_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        rows_manager_.start();
}

void history_database::commit()
{
    lookup_manager_.sync();
    rows_manager_.sync();
}

bool history_database::flush() const
{
    return
        lookup_file_.flush() &&
        rows_file_.flush();
}

bool history_database::close()
{
    return
        lookup_file_.close() &&
        rows_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

history_database::list history_database::get(const short_hash& key,
    size_t limit, size_t from_height) const
{
    list result;
    payment_record payment;
    const auto start = rows_multimap_.find(key);
    const auto records = record_multimap_iterable(rows_manager_, start);

    for (const auto index: records)
    {
        if (limit > 0 && result.size() >= limit)
            break;

        const auto record = rows_multimap_.get(index);
        auto deserial = make_unsafe_deserializer(record->buffer());

        // Failed reads are conflated with skipped returns.
        if (payment.from_data(deserial, from_height))
            result.push_back(payment);
    }

    return result;
}

history_statinfo history_database::statinfo() const
{
    return
    {
        lookup_header_.buckets(),
        lookup_manager_.count(),
        rows_manager_.count()
    };
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

    rows_multimap_.store(key, write);
}

// Update.
// ----------------------------------------------------------------------------

bool history_database::unlink_last_row(const short_hash& key)
{
    return rows_multimap_.unlink(key);
}

} // namespace database
} // namespace libbitcoin
