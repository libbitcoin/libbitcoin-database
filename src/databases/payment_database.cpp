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
#include <bitcoin/database/databases/payment_database.hpp>

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

// Total size of payment storage (using tx link vs. hash for point).
static const auto value_size = payment_record::satoshi_fixed_size(false);

// History uses a hash table index, O(1).
// The hash table stores indexes to the first element of unkeyed linked lists.
payment_database::payment_database(const path& lookup_filename,
    const path& rows_filename, size_t table_minimum, size_t index_minimum,
    uint32_t buckets, size_t expansion)
  : hash_table_file_(lookup_filename, table_minimum, expansion),

    // THIS sizeof(link_type) IS ASSUMED BY hash_table_multimap.
    hash_table_(hash_table_file_, buckets, sizeof(link_type)),

    // Linked-list storage for multimap.
    payment_index_file_(rows_filename, index_minimum, expansion),
    payment_index_(payment_index_file_, 0,
        hash_table_multimap<key_type, index_type, link_type>::size(value_size)),

    payment_multimap_(hash_table_, payment_index_)
{
    // TODO: C4267: 'argument': conversion from 'size_t' to 'Index', possible loss of data.
}

payment_database::~payment_database()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool payment_database::create()
{
    if (!hash_table_file_.open() ||
        !payment_index_file_.open())
        return false;

    // No need to call open after create.
    return
        hash_table_.create() &&
        payment_index_.create();
}

bool payment_database::open()
{
    return
        hash_table_file_.open() &&
        payment_index_file_.open() &&
        hash_table_.start() &&
        payment_index_.start();
}

void payment_database::commit()
{
    hash_table_.commit();
    payment_index_.commit();
}

bool payment_database::flush() const
{
    return
        hash_table_file_.flush() &&
        payment_index_file_.flush();
}

bool payment_database::close()
{
    return
        hash_table_file_.close() &&
        payment_index_file_.close();
}

// Queries.
// ----------------------------------------------------------------------------

payment_result payment_database::get(const hash_digest& hash) const
{
    // This does not populate hash or height, caller can dereference link.
    return { payment_multimap_.find(hash), hash };
}

// Store.
// ----------------------------------------------------------------------------

// protected
void payment_database::store(const hash_digest& key, const point& point,
    file_offset link, uint64_t value, bool output)
{
    const payment_record record
    {
        link,
        point.index(),
        value, // value or checksum
        output
    };

    const auto write = [&](byte_serializer& serial)
    {
        record.to_data(serial, false);
    };

    auto element = payment_multimap_.allocator();
    element.create(write);
    payment_multimap_.link(key, element);
}

// Confirmation of payment is dynamically derived from current tx state.
void payment_database::catalog(const transaction& tx)
{
    BITCOIN_ASSERT(tx.metadata.link);
    BITCOIN_ASSERT(!tx.metadata.cataloged);

    const auto tx_hash = tx.hash();
    const auto link = tx.metadata.link;
    const auto& inputs = tx.inputs();
    BITCOIN_ASSERT(inputs.size() <= max_uint32);

    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        const auto& input = inputs[index];
        const auto& prevout = input.previous_output();

        // Skip coinbase input.
        if (prevout.is_null())
            continue;

        BITCOIN_ASSERT(prevout.metadata.cache.is_valid());

        const input_point inpoint{ tx_hash, index };
        const auto& script = prevout.metadata.cache.script();
        const auto key = script.to_payments_key();
        store(key, inpoint, link, prevout.checksum(), false);
    }

    const auto& outputs = tx.outputs();
    BITCOIN_ASSERT(outputs.size() <= max_uint32);

    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto& output = outputs[index];
        const output_point outpoint{ tx_hash, index };
        const auto key = output.script().to_payments_key();
        store(key, outpoint, link, output.value(), true);
    }
}

} // namespace database
} // namespace libbitcoin
