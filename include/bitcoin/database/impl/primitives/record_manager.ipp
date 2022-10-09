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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_MANAGER_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_MANAGER_IPP

#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

/// [ record_count ]
/// [ record ]
/// ...
/// [ record ]

namespace libbitcoin {
namespace database {

// TODO: guard against overflows.
    
TEMPLATE
CLASS::record_manager(storage& file, size_t header_size,
    size_t record_size) NOEXCEPT
  : file_(file),
    header_size_(system::possible_narrow_cast<Link>(header_size)),
    record_size_(system::possible_narrow_cast<Link>(record_size)),
    record_count_(0)
{
    BC_ASSERT(header_size < not_allocated);
    BC_ASSERT(record_size < not_allocated);
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);

    // Existing file record count is nonzero.
    if (!is_zero(record_count_))
        return false;

    // Throws if there is insufficient space.
    file_.resize(header_size_ + link_to_position(record_count_));
    write_count();
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
bool CLASS::start() NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);

    read_count();
    const auto minimum = header_size_ + link_to_position(record_count_);

    // Records size does not exceed file size.
    return minimum <= file_.capacity();
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
void CLASS::commit() NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);
    write_count();
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return record_count_;
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
void CLASS::set_count(Link value) NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);
    BC_ASSERT(value <= record_count_);
    record_count_ = value;
    ///////////////////////////////////////////////////////////////////////////
}

// Return the next index, regardless of the number created.
// The file is thread safe, the critical section is to protect record_count_.
TEMPLATE
Link CLASS::allocate(size_t count) NOEXCEPT
{
    BC_ASSERT(count < not_allocated);
    const auto records = system::possible_narrow_cast<Link>(count);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);

    // Always write after the last index.
    const auto next_record_index = record_count_;
    const auto position = link_to_position(record_count_ + records);
    const auto required_size = header_size_ + position;

    // Currently throws runtime_error if insufficient space.
    if (!file_.reserve(required_size))
        return not_allocated;

    record_count_ += records;
    return next_record_index;
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
memory_ptr CLASS::get(Link link) const NOEXCEPT
{
    // Ensure requested position is within the file.
    // We avoid a runtime error here to optimize out the count lock.
    BC_ASSERT_MSG(!past_eof(link), "Read past end of file.");

    const auto memory = file_.access();
    memory->increment(header_size_ + link_to_position(link));
    return memory;
}

TEMPLATE
bool CLASS::past_eof(Link link) const NOEXCEPT
{
    return link >= count();
}

// privates

// Read the count value from the first 32 bits of the file after the header.
TEMPLATE
void CLASS::read_count() NOEXCEPT
{
    BC_ASSERT(header_size_ + sizeof(Link) <= file_.capacity());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_);
    record_count_ = system::unsafe_from_little_endian<Link>(memory->buffer());
}

// Write the count value to the first 32 bits of the file after the header.
TEMPLATE
void CLASS::write_count() NOEXCEPT
{
    BC_ASSERT(header_size_ + sizeof(Link) <= file_.capacity());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_);
    system::unsafe_to_little_endian<Link>(memory->buffer(), record_count_);
}

TEMPLATE
Link CLASS::position_to_link(file_offset position) const NOEXCEPT
{
    return (position - sizeof(Link)) / record_size_;
}

TEMPLATE
file_offset CLASS::link_to_position(Link link) const NOEXCEPT
{
    return sizeof(Link) + link * record_size_;
}

} // namespace database
} // namespace libbitcoin

#endif
