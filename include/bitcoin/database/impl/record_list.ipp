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
#ifndef LIBBITCOIN_DATABASE_RECORD_LIST_IPP
#define LIBBITCOIN_DATABASE_RECORD_LIST_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {
    
// Parameterizing RecordManager allows const and non-const.
template <typename LinkType, typename RecordManager>
record_list<LinkType, RecordManager>::record_list(RecordManager& manager)
    : manager_(manager), index_(not_found)
{
}

template <typename LinkType, typename RecordManager>
record_list<LinkType, RecordManager>::record_list(RecordManager& manager,
    LinkType index)
  : manager_(manager), index_(index)
{
}

template <typename LinkType, typename RecordManager>
LinkType record_list<LinkType, RecordManager>::create(write_function write)
{
    BITCOIN_ASSERT(index_ == not_found);

    // Create new record without populating its next pointer.
    //   [ next:4   ]
    //   [ value... ] <==
    index_ = manager_.allocate(1);

    const auto memory = raw_data(sizeof(LinkType));
    const auto record = memory->buffer();
    auto serial = make_unsafe_serializer(record);
    serial.write_delegated(write);
    return index_;
}

template <typename LinkType, typename RecordManager>
void record_list<LinkType, RecordManager>::link(LinkType next)
{
    // Populate next pointer value.
    //   [ next:4   ] <==
    //   [ value... ]

    // Write record.
    const auto memory = raw_data(0);
    auto serial = make_unsafe_serializer(memory->buffer());

    // Link reads and writes are presumed to be protected by multimap.
    //*************************************************************************
    serial.template write_little_endian<LinkType>(next);
    //*************************************************************************
}

template <typename LinkType, typename RecordManager>
memory_ptr record_list<LinkType, RecordManager>::data() const
{
    // Get value pointer.
    //   [ next:4   ]
    //   [ value... ] ==>

    // Value data is at the end.
    return raw_data(sizeof(LinkType));
}

template <typename LinkType, typename RecordManager>
LinkType record_list<LinkType, RecordManager>::next() const
{
    const auto memory = raw_data(0);

    // Link reads and writes are presumed to be protected by multimap.
    //*************************************************************************
    return from_little_endian_unsafe<LinkType>(memory->buffer());
    //*************************************************************************
}

template <typename LinkType, typename RecordManager>
memory_ptr record_list<LinkType, RecordManager>::raw_data(
    file_offset bytes) const
{
    auto memory = manager_.get(index_);
    memory->increment(bytes);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
