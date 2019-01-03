/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_RECORD_MANAGER_IPP
#define LIBBITCOIN_DATABASE_RECORD_MANAGER_IPP

#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

/// [ record_count ]
/// [ record ]
/// ...
/// [ record ]

namespace libbitcoin {
namespace database {

// TODO: guard against overflows.

template <typename Link>
record_manager<Link>::record_manager(storage& file, size_t header_size,
    size_t record_size)
  : file_(file),
    header_size_(header_size),
    record_size_(record_size),
    record_count_(0)
{
}

template <typename Link>
bool record_manager<Link>::create()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    system::unique_lock lock(mutex_);

    // Existing file record count is nonzero.
    if (record_count_ != 0)
        return false;

    // This currently throws if there is insufficient space.
    file_.resize(header_size_ + link_to_position(record_count_));
    write_count();
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Link>
bool record_manager<Link>::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    system::unique_lock lock(mutex_);

    read_count();
    const auto minimum = header_size_ + link_to_position(record_count_);

    // Records size does not exceed file size.
    return minimum <= file_.capacity();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Link>
void record_manager<Link>::commit()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    system::unique_lock lock(mutex_);
    write_count();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Link>
Link record_manager<Link>::count() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    system::shared_lock lock(mutex_);
    return record_count_;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Link>
void record_manager<Link>::set_count(Link value)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    system::unique_lock lock(mutex_);
    BITCOIN_ASSERT(value <= record_count_);
    record_count_ = value;
    ///////////////////////////////////////////////////////////////////////////
}

// Return the next index, regardless of the number created.
// The file is thread safe, the critical section is to protect record_count_.
template <typename Link>
Link record_manager<Link>::allocate(size_t count)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    system::unique_lock lock(mutex_);

    // Always write after the last index.
    const auto next_record_index = record_count_;
    const size_t position = link_to_position(record_count_ + count);
    const size_t required_size = header_size_ + position;

    // Currently throws runtime_error if insufficient space.
    if (!file_.reserve(required_size))
        return 0;

    record_count_ += count;
    return next_record_index;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Link>
memory_ptr record_manager<Link>::get(Link link) const
{
    // If record >= count() then we should still be within the file. The
    // condition implies a block has been unconfirmed while reading it.

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_ + link_to_position(link));
    return memory;
}

// privates

// Read the count value from the first 32 bits of the file after the header.
template <typename Link>
void record_manager<Link>::read_count()
{
    BITCOIN_ASSERT(header_size_ + sizeof(Link) <= file_.capacity());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_);
    auto deserial = system::make_unsafe_deserializer(memory->buffer());
    record_count_ = deserial.template read_little_endian<Link>();
}

// Write the count value to the first 32 bits of the file after the header.
template <typename Link>
void record_manager<Link>::write_count()
{
    BITCOIN_ASSERT(header_size_ + sizeof(Link) <= file_.capacity());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_);
    auto serial = system::make_unsafe_serializer(memory->buffer());
    serial.template write_little_endian<Link>(record_count_);
}

template <typename Link>
Link record_manager<Link>::position_to_link(file_offset position) const
{
    return (position - sizeof(Link)) / record_size_;
}

template <typename Link>
file_offset record_manager<Link>::link_to_position(Link link) const
{
    return sizeof(Link) + link * record_size_;
}

} // namespace database
} // namespace libbitcoin

#endif
