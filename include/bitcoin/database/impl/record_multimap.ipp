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
#ifndef LIBBITCOIN_DATABASE_RECORD_MULTIMAP_IPP
#define LIBBITCOIN_DATABASE_RECORD_MULTIMAP_IPP

#include <string>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

template <typename KeyType>
record_multimap<KeyType>::record_multimap(record_hash_table_type& map,
    record_list& records)
  : map_(map), records_(records)
{
}

template <typename KeyType>
array_index record_multimap<KeyType>::lookup(const KeyType& key) const
{
    const auto start_info = map_.find(key);

    if (!start_info)
        return records_.empty;

    // Read the existing row start of the key (map_.read).
    const auto address = REMAP_ADDRESS(start_info);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_shared();
    const auto old_begin = from_little_endian_unsafe<array_index>(address);
    mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    BITCOIN_ASSERT(old_begin != records_.empty);
    return old_begin;
}

//=============================================================================
// TODO: find and create_new require mutual exclusivity so that we do not
// create two new entries. Otherwise create_new requires exclusivity and an
// internal reexecution of find. This would prevent need to lock find for
// queries, although it would only be impacted by concurrent write/delete.
//=============================================================================
template <typename KeyType>
void record_multimap<KeyType>::add_row(const KeyType& key,
    write_function write)
{
    const auto start_info = map_.find(key);

    if (!start_info)
    {
        // Create a new first record for the key.
        const auto new_begin = records_.create();
        const auto memory = records_.get(new_begin);
        const auto data = REMAP_ADDRESS(memory);
        auto serial_record = make_unsafe_serializer(data);
        serial_record.write_delegated(write);

        // records_ and start_info remap safe pointers are in distinct files.
        const auto write_start_info = [&](serializer<uint8_t*>& serial)
        {
            //*****************************************************************
            serial.template write_little_endian<array_index>(new_begin);
            //*****************************************************************
        };

        // Associate the new row start with the key.
        map_.store(key, write_start_info);
        return;
    }

    // Read the existing row start of the key (map_.read).
    const auto address = REMAP_ADDRESS(start_info);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_shared();
    const auto old_begin = from_little_endian_unsafe<array_index>(address);
    mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    BITCOIN_ASSERT(old_begin != records_.empty);
    const auto new_begin = records_.insert(old_begin);
    const auto memory = records_.get(new_begin);
    const auto data = REMAP_ADDRESS(memory);
    auto serial_record = make_unsafe_serializer(data);
    serial_record.write_delegated(write);

    // Associate the new row start with the key (map_.update).
    // records_ and start_info remap safe pointers are in distinct files.
    auto serial_link = make_unsafe_serializer(address);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock();
    serial_link.template write_little_endian<array_index>(new_begin);
    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename KeyType>
bool record_multimap<KeyType>::delete_last_row(const KeyType& key)
{
    const auto start_info = map_.find(key);

    if (!start_info)
        return false;

    // Read the existing row start of the key (map_.read).
    auto address = REMAP_ADDRESS(start_info);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_shared();
    const auto old_begin = from_little_endian_unsafe<array_index>(address);
    mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    BITCOIN_ASSERT(old_begin != records_.empty);
    const auto new_begin = records_.next(old_begin);

    if (new_begin == records_.empty)
    {
        // Free existing remap pointer to prevent deadlock in map_.unlink.
        address = nullptr;
        return map_.unlink(key);
    }

    auto serial = make_unsafe_serializer(address);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock();
    serial.template write_little_endian<array_index>(new_begin);
    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
