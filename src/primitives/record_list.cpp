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
#include <bitcoin/database/primitives/record_list.hpp>

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

record_list::record_list(record_manager& manager, array_index index)
  : manager_(manager), index_(index)
{
}

array_index record_list::create(write_function write)
{
    BITCOIN_ASSERT(index_ == empty);

    // Create new record without populating its next pointer.
    //   [ next:4   ]
    //   [ value... ] <==
    index_ = manager_.new_records(1);

    const auto memory = raw_data(index_size);
    const auto record = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(record);
    serial.write_delegated(write);

    return index_;
}

void record_list::link(array_index next)
{
    // Populate next pointer value.
    //   [ next:4   ] <==
    //   [ value... ]

    // Write record.
    const auto memory = raw_data(0);
    const auto next_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(next_data);

    //*************************************************************************
    serial.template write_little_endian<array_index>(next);
    //*************************************************************************
}
memory_ptr record_list::data() const
{
    // Get value pointer.
    //   [ next:4   ]
    //   [ value... ] ==>

    // Value data is at the end.
    return raw_data(index_size);
}

array_index record_list::next_index() const
{
    const auto memory = raw_data(0);
    const auto next_address = REMAP_ADDRESS(memory);
    //*************************************************************************
    return from_little_endian_unsafe<array_index>(next_address);
    //*************************************************************************
}

memory_ptr record_list::raw_data(file_offset offset) const
{
    auto memory = manager_.get(index_);
    REMAP_INCREMENT(memory, offset);
    return memory;
}

} // namespace database
} // namespace libbitcoin
