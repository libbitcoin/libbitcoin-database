/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

// Sponsored in part by Digital Contract Design, LLC

#include <bitcoin/database/databases/filter_database.hpp>

#include <cstddef>
#include <cstdint>
#include <boost/filesystem.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/filter_result.hpp>

namespace libbitcoin {
namespace database {

// Record format (v4):
// ----------------------------------------------------------------------------
// [ filter_header:32    - const    ]
// [ filter:varint       - const    ]

static constexpr auto filter_header_size = system::hash_size;

// Filter uses a hash table index, O(1).
filter_database::filter_database(const path& map_filename,
    size_t table_minimum, size_t buckets, size_t expansion,
    uint8_t filter_type)
  : filter_type_(filter_type),
    hash_table_file_(map_filename, table_minimum, expansion),
    hash_table_(hash_table_file_, buckets)
{
}

filter_database::~filter_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool filter_database::create()
{
    if (!hash_table_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create();
}

bool filter_database::open()
{
    return
        hash_table_file_.open() &&
        hash_table_.start();
}

void filter_database::commit()
{
    hash_table_.commit();
}

bool filter_database::flush() const
{
    return hash_table_file_.flush();
}

bool filter_database::close()
{
    return hash_table_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

filter_result filter_database::get(file_offset link) const
{
    // This is not guarded for an invalid offset.
    return { hash_table_.get(link), metadata_mutex_, filter_type_ };
}

filter_result filter_database::get(const system::hash_digest& hash) const
{
    return { hash_table_.find(hash), metadata_mutex_, filter_type_ };
}

// Store.
// ----------------------------------------------------------------------------

// Store new filter_data.
bool filter_database::store(const system::hash_digest& block_hash,
    const system::chain::block_filter& block_filter)
{
    if (block_filter.filter_type() != filter_type_)
        return false;

    return storize(block_hash, block_filter);
}

// private
bool filter_database::storize(const system::hash_digest& block_hash,
    const system::chain::block_filter& block_filter)
{
    const auto writer = [&](byte_serializer& serial)
    {
        block_filter.to_data(serial, false);
    };

    // Transactions are variable-sized.
    const auto size = block_filter.serialized_size(false);

    // Write the new transaction.
    auto next = hash_table_.allocator();
    block_filter.metadata.link = next.create(block_hash, writer, size);
    hash_table_.link(next);
    return true;
}

} // namespace database
} // namespace libbitcoin
