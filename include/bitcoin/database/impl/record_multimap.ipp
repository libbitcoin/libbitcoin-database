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

#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_list.hpp>

namespace libbitcoin {
namespace database {

// static
template <typename KeyType, typename LinkType>
size_t record_multimap<KeyType, LinkType>::element_size(size_t value_size)
{
    return sizeof(LinkType) + value_size;
}

template <typename KeyType, typename LinkType>
record_multimap<KeyType, LinkType>::record_multimap(
    record_hash_table_type& map, record_manager& manager)
  : map_(map), manager_(manager)
{
}

template <typename KeyType, typename LinkType>
void record_multimap<KeyType, LinkType>::store(const KeyType& key,
    write_function write)
{
    // Allocate and populate new unlinked row.
    record_list<LinkType> record(manager_);
    const auto begin = record.create(write);

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(create_mutex_);

    const auto old_begin = find(key);

    // Link the row to the previous first element (or terminator).
    record.link(old_begin);

    if (old_begin == record_list<LinkType>::empty)
    {
        map_.store(key, [=](serializer<uint8_t*>& serial)
        {
            //*****************************************************************
            serial.template write_little_endian<LinkType>(begin);
            //*****************************************************************
        });
    }
    else
    {
        map_.update(key, [=](serializer<uint8_t*>& serial)
        {
            // Critical Section
            ///////////////////////////////////////////////////////////////////
            unique_lock lock(update_mutex_);
            serial.template write_little_endian<LinkType>(begin);
            ///////////////////////////////////////////////////////////////////
        });
    }
    ///////////////////////////////////////////////////////////////////////////
}

template <typename KeyType, typename LinkType>
LinkType record_multimap<KeyType, LinkType>::find(const KeyType& key) const
{
    const auto begin_address = map_.find(key);

    if (!begin_address)
        return record_list<LinkType>::empty;

    const auto memory = begin_address->buffer();

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(update_mutex_);
    return from_little_endian_unsafe<LinkType>(memory);
    ///////////////////////////////////////////////////////////////////////////
}

/// Get a remap safe address pointer to the indexed data.
template <typename KeyType, typename LinkType>
memory_ptr record_multimap<KeyType, LinkType>::get(LinkType index) const
{
    const record_list<LinkType> record(manager_, index);
    return record.data();
}

// Unlink is not safe for concurrent write.
template <typename KeyType, typename LinkType>
bool record_multimap<KeyType, LinkType>::unlink(const KeyType& key)
{
    const auto begin = find(key);

    // No rows exist.
    if (begin == record_list<LinkType>::empty)
        return false;

    const auto next_index = record_list<LinkType>(manager_, begin)
        .next_index();

    // Remove the hash table entry, which delinks the single row.
    if (next_index == record_list<LinkType>::empty)
        return map_.unlink(key);

    // Update the hash table entry, which skips the first of multiple rows.
    map_.update(key, [&](serializer<uint8_t*>& serial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(update_mutex_);
        serial.template write_little_endian<LinkType>(next_index);
        ///////////////////////////////////////////////////////////////////////
    });

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif