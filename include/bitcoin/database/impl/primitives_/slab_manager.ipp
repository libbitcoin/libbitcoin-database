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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_SLAB_MANAGER_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_SLAB_MANAGER_IPP

#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

/// [ payload_size ] (includes self)
/// [ slab ]
/// ...
/// [ slab ]

namespace libbitcoin {
namespace database {

// TODO: guard against overflows.

TEMPLATE
CLASS::slab_manager(storage& file, size_t header_size) NOEXCEPT
  : file_(file),
    header_size_(system::possible_narrow_cast<Link>(header_size)),
    payload_size_(sizeof(Link))
{
    BC_ASSERT(header_size < not_allocated);
    BC_ASSERT(sizeof(Link) < not_allocated);
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);

    // Existing slabs size is incorrect for new file.
    if (payload_size_ != sizeof(Link))
        return false;

    // This currently throws if there is insufficient space.
    file_.resize(header_size_ + payload_size_);
    write_size();
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
bool CLASS::start() NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);

    read_size();
    const auto minimum = header_size_ + payload_size_;

    // Slabs size does not exceed file size.
    return minimum <= file_.capacity();
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
void CLASS::commit() NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);
    write_size();
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
Link CLASS::payload_size() const NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return payload_size_;
    ///////////////////////////////////////////////////////////////////////////
}

// Return is offset by header but not size storage (embedded in data files).
// The file is thread safe, the critical section is to protect payload_size_.
TEMPLATE
Link CLASS::allocate(size_t size) NOEXCEPT
{
    BC_ASSERT(size < not_allocated);
    const auto slab_size = system::possible_narrow_cast<Link>(size);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);

    // Always write after the last slab.
    const auto next_slab_position = payload_size_;
    const auto required_size = header_size_ + payload_size_ + slab_size;

    // Throws runtime_error if insufficient space.
    if (!file_.reserve(required_size))
        return not_allocated;

    payload_size_ += slab_size;
    return next_slab_position;
    ///////////////////////////////////////////////////////////////////////////
}

// Position is offset by header but not size storage (embedded in data files).
TEMPLATE
memory_ptr CLASS::get(Link link) const NOEXCEPT
{
    // Ensure requested position is within the file.
    // We avoid a runtime error here to optimize out the payload_size lock.
    BC_ASSERT_MSG(link < payload_size(), "Read past end of file.");

    const auto memory = file_.access();
    memory->increment(header_size_ + link);
    return memory;
}

TEMPLATE
bool CLASS::past_eof(Link link) const NOEXCEPT
{
    return link >= payload_size();
}

// privates

// Read the size value from the first 64 bits of the file after the header.
TEMPLATE
void CLASS::read_size() NOEXCEPT
{
    BC_ASSERT(header_size_ + sizeof(Link) <= file_.capacity());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_);
    payload_size_ = system::unsafe_from_little_endian<Link>(memory->buffer());
}

// Write the size value to the first 64 bits of the file after the header.
TEMPLATE
void CLASS::write_size() const NOEXCEPT
{
    BC_ASSERT(header_size_ + sizeof(Link) <= file_.capacity());

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(header_size_);
    system::unsafe_to_little_endian<Link>(memory->buffer(), payload_size_);
}

} // namespace database
} // namespace libbitcoin

#endif
