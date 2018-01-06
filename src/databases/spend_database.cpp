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
#include <bitcoin/database/databases/spend_database.hpp>

#include <cstddef>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_hash_table.hpp>
#include <bitcoin/database/primitives/record_row.hpp>

// Record format (v3):
// ----------------------------------------------------------------------------
// [ point-hash:32 - const]
// [ point-index:2 - const]

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static const auto value_size = std::tuple_size<point>::value;

// Spends use a hash table index, O(1).
// The spend database keys off of output point and has input point value.
spend_database::spend_database(const path& filename, size_t buckets,
    size_t expansion)
  : lookup_file_(filename, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_,
        hash_table_header<index_type, link_type>::size(buckets),
        record_row<key_type, array_index>::size(value_size)),
    lookup_map_(lookup_header_, lookup_manager_)
{
}

spend_database::~spend_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool spend_database::create()
{
    if (!lookup_file_.open())
        return false;

    // No need to call open after create.
    return
        lookup_header_.create() &&
        lookup_manager_.create();
}

bool spend_database::open()
{
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

void spend_database::commit()
{
    lookup_manager_.sync();
}

bool spend_database::flush() const
{
    return lookup_file_.flush();
}

bool spend_database::close()
{
    return lookup_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

input_point spend_database::get(const output_point& outpoint) const
{
    input_point spend;
    const auto slab = lookup_map_.find(outpoint);

    if (!slab)
        return spend;

    auto deserial = make_unsafe_deserializer(slab->buffer());
    spend.from_data(deserial, false);
    return spend;
}

spend_statinfo spend_database::statinfo() const
{
    return
    {
        lookup_header_.buckets(),
        lookup_manager_.count()
    };
}

// Store.
// ----------------------------------------------------------------------------

void spend_database::store(const chain::output_point& outpoint,
    const chain::input_point& spend)
{
    const auto write = [&](byte_serializer& serial)
    {
        spend.to_data(serial, false);
    };

    lookup_map_.store(outpoint, write);
}

// Update.
// ----------------------------------------------------------------------------

bool spend_database::unlink(const output_point& outpoint)
{
    auto memory = lookup_map_.find(outpoint);

    // Spends are optional so do not assume presence.
    if (memory == nullptr)
        return false;

    // Release lock before unlinking.
    memory = nullptr;
    return lookup_map_.unlink(outpoint);
}

} // namespace database
} // namespace libbitcoin
