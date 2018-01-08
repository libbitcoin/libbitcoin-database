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
#ifndef LIBBITCOIN_DATABASE_RECORD_MANAGER_IPP
#define LIBBITCOIN_DATABASE_RECORD_MANAGER_IPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

/// -- file --
/// [ header ]
/// [ record_count ]
/// [ payload ]

/// -- header (hash table) --
/// [ count  ]
/// [ bucket ]
/// ...
/// [ bucket ]

/// -- payload (fixed size records) --
/// [ record ]
/// ...
/// [ record ]

namespace libbitcoin {
namespace database {

// TODO: guard against overflows.

template <typename LinkType>
record_manager<LinkType>::record_manager(storage& file, size_t header_size,
    size_t record_size)
  : file_(file),
    header_size_(header_size),
    record_size_(record_size),
    record_count_(0)
{
}

template <typename LinkType>
bool record_manager<LinkType>::create()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // Existing file record count is nonzero.
    if (record_count_ != 0)
        return false;

    // This currently throws if there is insufficient space.
    file_.resize(header_size_ + record_to_position(record_count_));

    write_count();
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename LinkType>
bool record_manager<LinkType>::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    read_count();
    const auto minimum = header_size_ + record_to_position(record_count_);

    // Records size exceeds file size.
    return minimum <= file_.size();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename LinkType>
void record_manager<LinkType>::sync()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    write_count();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename LinkType>
LinkType record_manager<LinkType>::count() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    return record_count_;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename LinkType>
void record_manager<LinkType>::set_count(const LinkType value)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    BITCOIN_ASSERT(value <= record_count_);

    record_count_ = value;
    ///////////////////////////////////////////////////////////////////////////
}

// Return the next index, regardless of the number created.
// The file is thread safe, the critical section is to protect record_count_.
template <typename LinkType>
LinkType record_manager<LinkType>::allocate(size_t count)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // Always write after the last index.
    const auto next_record_index = record_count_;

    const size_t position = record_to_position(record_count_ + count);
    const size_t required_size = header_size_ + position;

    if (!file_.reserve(required_size))
    {
        // TODO: return failure sentinel.
    }

    record_count_ += count;

    return next_record_index;
    ///////////////////////////////////////////////////////////////////////////
}

template <typename LinkType>
memory_ptr record_manager<LinkType>::get(LinkType record) const
{
    // If record >= count() then we should still be within the file. The
    // condition implies a block has been unconfirmed while reading it.

    // The accessor must remain in scope until the end of the block.
    auto memory = file_.access();
    memory->increment(header_size_ + record_to_position(record));
    return memory;
}

// privates

// Read the count value from the first 32 bits of the file after the header.
template <typename LinkType>
void record_manager<LinkType>::read_count()
{
    BITCOIN_ASSERT(header_size_ + sizeof(LinkType) <= file_.size());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto count_address = memory->buffer() + header_size_;
    record_count_ = from_little_endian_unsafe<LinkType>(count_address);
}

// Write the count value to the first 32 bits of the file after the header.
template <typename LinkType>
void record_manager<LinkType>::write_count()
{
    BITCOIN_ASSERT(header_size_ + sizeof(LinkType) <= file_.size());

    // The accessor must remain in scope until the end of the block.
    auto memory = file_.access();
    auto payload_size_address = memory->buffer() + header_size_;
    auto serial = make_unsafe_serializer(payload_size_address);
    serial.write_little_endian<LinkType>(record_count_);
}

template <typename LinkType>
LinkType record_manager<LinkType>::position_to_record(
    file_offset position) const
{
    return (position - sizeof(LinkType)) / record_size_;
}

template <typename LinkType>
file_offset record_manager<LinkType>::record_to_position(LinkType record) const
{
    return sizeof(LinkType) + record * record_size_;
}

} // namespace database
} // namespace libbitcoin

#endif
