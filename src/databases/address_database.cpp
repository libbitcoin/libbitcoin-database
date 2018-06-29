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

// Record format (v4/v3) [47 bytes, 71 with key/link]:
// ----------------------------------------------------------------------------
// [ kind:1        - const]
// [ point-hash:32 - const]
// [ point-index:2 - const]
// [ height:4      - const]
// [ checksum:8    - const]

namespace libbitcoin {
namespace database {

using namespace bc::chain;

// Total size of address storage (using tx link vs. hash for point).
static const auto value_size = payment_record::satoshi_fixed_size(false);

// History uses a hash table index, O(1).
// The hash table stores indexes to the first element of unkeyed linked lists.
address_database::address_database(const path& lookup_filename,
    const path& rows_filename, size_t buckets, size_t expansion)
  : hash_table_file_(lookup_filename, expansion),

    // THIS sizeof(link_type) IS ASSUMED BY hash_table_multimap.
    hash_table_(hash_table_file_, buckets, sizeof(link_type)),

    // Linked-list storage for multimap.
    address_index_file_(rows_filename, expansion),
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
address_result address_database::get(const short_hash& hash) const
{
    return { address_multimap_.find(hash), hash };
}

// Store.
// ----------------------------------------------------------------------------

// Confirmation of payment is dynamically derived from current tx state.
void address_database::index(const chain::transaction& tx)
{
    // TODO: loop over payments, relying only on output scripts, adding rows.
    ////const auto writer = [&](byte_serializer& serial)
    ////{
    ////    payment.to_data(serial, false);
    ////};

    ////// Write the new payment history.
    ////auto next = address_multimap_.allocator();
    ////next.create(writer);
    ////address_multimap_.link(hash, next);
}

} // namespace database
} // namespace libbitcoin
