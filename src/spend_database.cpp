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
#include <bitcoin/database/spend_database.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

BC_CONSTEXPR size_t value_size = hash_size + 4;
BC_CONSTEXPR size_t record_size = hash_table_record_size<hash_digest>(value_size);

// Create a new hash from a hash + index (a point)
// deterministically suitable for use in a hashtable.
// This technique could be replaced by simply using the output.hash.
static hash_digest output_to_hash(const chain::output_point& output)
{
    data_chunk point(sizeof(output.index) + sizeof(output.hash));
    const auto index = to_little_endian(output.index);
    std::copy(output.hash.begin(), output.hash.end(), point.begin());
    std::copy(index.begin(), index.end(), point.begin() + sizeof(output.hash));

    // The index has a *very* low level of bit distribution evenness, 
    // almost none, and we must preserve the presumed random bit distribution,
    // so we need to re-hash here.
    return sha256_hash(point);
}

spend_database::spend_database(const path& filename)
  : file_(filename), 
    header_(file_, number_buckets),
    manager_(file_, header_size, record_size),
    map_(header_, manager_)
{
    BITCOIN_ASSERT(ADDRESS(file_.access()) != nullptr);
}

void spend_database::create()
{
    file_.allocate(initial_map_file_size);
    header_.create();
    manager_.create();
}

void spend_database::start()
{
    header_.start();
    manager_.start();
}

bool spend_database::stop()
{
    return file_.stop();
}

spend spend_database::get(const output_point& outpoint) const
{
    const auto key = output_to_hash(outpoint);
    const auto memory = map_.find(key);

    if (!memory)
        return { false, 0, {} };

    hash_digest hash;
    const auto record = ADDRESS(memory);
    std::copy(record, record + hash_size, hash.begin());
    const auto index = from_little_endian_unsafe<uint32_t>(record + hash_size);

    return
    {
        true,
        index,
        hash
    };
}

void spend_database::store(const chain::output_point& outpoint,
    const chain::input_point& spend)
{
    const auto write = [&spend](uint8_t* data)
    {
        auto serial = make_serializer(data);
        auto raw_spend = spend.to_data();
        serial.write_data(raw_spend);
    };

    const auto key = output_to_hash(outpoint);
    map_.store(key, write);
}

void spend_database::remove(const output_point& outpoint)
{
    const auto key = output_to_hash(outpoint);
    DEBUG_ONLY(bool success =) map_.unlink(key);
    BITCOIN_ASSERT(success);
}

void spend_database::sync()
{
    manager_.sync();
}

spend_statinfo spend_database::statinfo() const
{
    return
    {
        header_.size(),
        manager_.count()
    };
}

} // namespace database
} // namespace libbitcoin
