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
#include <bitcoin/database/databases/address_database.hpp>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/hash_table_multimap.hpp>

// Record format (v4/v3) [47 bytes, 71 with key/link]:
// ----------------------------------------------------------------------------
// [ kind:1        - const]
// [ point-hash:32 - const]
// [ point-index:2 - const]
// [ height:4      - const]
// [ checksum:8    - const]

namespace libbitcoin {
namespace database {

using namespace bc::system;
using namespace bc::system::chain;

// Total size of address storage (using tx link vs. hash for point).
static const auto value_size = payment_record::satoshi_fixed_size(false);

// History uses a hash table index, O(1).
// The hash table stores indexes to the first element of unkeyed linked lists.
address_database::address_database(const path& lookup_filename,
    const path& rows_filename, size_t table_minimum, size_t index_minimum,
    size_t buckets, size_t expansion)
  : hash_table_file_(lookup_filename, table_minimum, expansion),

    // THIS sizeof(link_type) IS ASSUMED BY hash_table_multimap.
    hash_table_(hash_table_file_, buckets, sizeof(link_type)),

    // Linked-list storage for multimap.
    address_index_file_(rows_filename, index_minimum, expansion),
    address_index_(address_index_file_, 0,
        hash_table_multimap<key_type, index_type, link_type>::size(value_size)),

    address_multimap_(hash_table_, address_index_)
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
        !address_index_file_.open())
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
        address_index_file_.open() &&
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
        address_index_file_.flush();
}

bool address_database::close()
{
    return
        hash_table_file_.close() &&
        address_index_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

// TODO: obtain confirmation height from tx record (along with hash).
address_result address_database::get(const hash_digest& hash) const
{
    return { address_multimap_.find(hash), hash };
}

// Store.
// ----------------------------------------------------------------------------

// protected
void address_database::store(const hash_digest& script_hash, const point& point,
    file_offset link, bool output)
{
    const payment_record record
    {
        link,
        point.index(),
        point.checksum(),
        output
    };

    const auto write = [&](byte_serializer& serial)
    {
        record.to_data(serial, false);
    };

    auto element = address_multimap_.allocator();
    element.create(write);
    address_multimap_.link(script_hash, element);
}

// Confirmation of payment is dynamically derived from current tx state.
void address_database::catalog(const transaction& tx)
{
    BITCOIN_ASSERT(tx.metadata.link);
    BITCOIN_ASSERT(!tx.metadata.existed);
    BITCOIN_ASSERT(outputs.size() < max_uint32);

    const auto tx_hash = tx.hash();
    const auto link = tx.metadata.link;
    const auto& inputs = tx.inputs();
    BITCOIN_ASSERT(inputs.size() < max_uint32);

    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        const auto& input = inputs[index];

        // Skip coinbase input.
        if (input.previous_output().is_null())
            continue;

        BITCOIN_ASSERT(input.previous_output().metadata.cache.is_valid());

        const input_point inpoint{ tx_hash, index };
        const auto& script = input.previous_output().metadata.cache.script();
        const auto script_hash = sha256_hash(script.to_data(false));
        store(script_hash, inpoint, link, false);
    }

    const auto& outputs = tx.outputs();
    BITCOIN_ASSERT(outputs.size() < max_uint32);

    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto& output = outputs[index];
        const output_point outpoint{ tx_hash, index };
        const auto& script = output.script();
        const auto script_hash = sha256_hash(script.to_data(false));
        store(script_hash, outpoint, link, true);
    }
}

} // namespace database
} // namespace libbitcoin
