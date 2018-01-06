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
#ifndef LIBBITCOIN_DATABASE_RECORD_ROW_IPP
#define LIBBITCOIN_DATABASE_RECORD_ROW_IPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

// static
template <typename KeyType, typename LinkType>
size_t record_row<KeyType, LinkType>::size(size_t value_size)
{
    return std::tuple_size<KeyType>::value + sizeof(LinkType) + value_size;
}

template <typename KeyType, typename LinkType>
record_row<KeyType, LinkType>::record_row(record_manager& manager)
  : record_row(manager, not_found)
{
}

template <typename KeyType, typename LinkType>
record_row<KeyType, LinkType>::record_row(record_manager& manager,
    LinkType index)
  : manager_(manager), index_(index)
{
}

template <typename KeyType, typename LinkType>
LinkType record_row<KeyType, LinkType>::create(const KeyType& key,
    write_function write)
{
    BITCOIN_ASSERT(index_ == not_found);

    // Create new (unlinked) record and populate its key and data.
    //   [ KeyType  ] <==
    //   [ next:4   ]
    //   [ value... ] <==
    index_ = manager_.new_records(1);

    const auto memory = raw_data(key_start);
    const auto record = memory->buffer();
    auto serial = make_unsafe_serializer(record);
    serial.write_forward(key);
    serial.skip(link_size);
    serial.write_delegated(write);
    return index_;
}

template <typename KeyType, typename LinkType>
void record_row<KeyType, LinkType>::link(LinkType next)
{
    // Populate next pointer value.
    //   [ KeyType  ]
    //   [ next:4   ] <==
    //   [ value... ]

    // Write record.
    const auto memory = raw_data(key_size);
    const auto next_data = memory->buffer();
    auto serial = make_unsafe_serializer(next_data);

    //*************************************************************************
    serial.template write_little_endian<array_index>(next);
    //*************************************************************************
}

template <typename KeyType, typename LinkType>
bool record_row<KeyType, LinkType>::compare(const KeyType& key) const
{
    // Key data is at the start.
    const auto memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), memory->buffer());
}

template <typename KeyType, typename LinkType>
memory_ptr record_row<KeyType, LinkType>::data() const
{
    // Get value pointer.
    //   [ KeyType  ]
    //   [ next:4   ]
    //   [ value... ] ==>

    // Value data is at the end.
    return raw_data(prefix_size);
}

template <typename KeyType, typename LinkType>
file_offset record_row<KeyType, LinkType>::offset() const
{
    // Value data is at the end.
    return index_ + prefix_size;
}

template <typename KeyType, typename LinkType>
LinkType record_row<KeyType, LinkType>::next_index() const
{
    const auto memory = raw_data(key_size);
    const auto next_address = memory->buffer();

    //*************************************************************************
    return from_little_endian_unsafe<array_index>(next_address);
    //*************************************************************************
}

template <typename KeyType, typename LinkType>
void record_row<KeyType, LinkType>::write_next_index(LinkType next)
{
    const auto memory = raw_data(key_size);
    auto serial = make_unsafe_serializer(memory->buffer());

    //*************************************************************************
    serial.template write_little_endian<array_index>(next);
    //*************************************************************************
}

template <typename KeyType, typename LinkType>
memory_ptr record_row<KeyType, LinkType>::raw_data(size_t bytes) const
{
    auto memory = manager_.get(index_);
    memory->increment(bytes);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
