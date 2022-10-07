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
#ifndef LIBBITCOIN_DATABASE_SLAB_MANAGER_HPP
#define LIBBITCOIN_DATABASE_SLAB_MANAGER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// The slab manager represents a growing collection of various sized
/// slabs of data on disk. It will resize the file accordingly and keep
/// track of the current end pointer so new slabs can be allocated.
template <typename Link,
    if_unsigned_integer<Link> = true>
class slab_manager
{
public:
    static const Link not_allocated =
        system::possible_narrow_cast<Link>(max_uint64);

    slab_manager(storage& file, size_t header_size) NOEXCEPT;

    /// Create slab manager.
    bool create() NOEXCEPT;

    /// Prepare manager for use.
    bool start() NOEXCEPT;

    /// Commit total slabs size to the file.
    void commit() NOEXCEPT;

    /// Get the size of all slabs and size prefix (excludes header).
    Link payload_size() const NOEXCEPT;

    /// Allocate a slab and return its position, commit after writing.
    Link allocate(size_t size) NOEXCEPT;

    /// Check if link is past eof
    bool past_eof(Link link) const NOEXCEPT;

    /// Return memory object for the slab at the specified position.
    memory_ptr get(Link position) const NOEXCEPT;

private:
    // Read the size of the data from the file.
    void read_size() NOEXCEPT;

    // Write the size of the data from the file.
    void write_size() const NOEXCEPT;

    // This class is thread and remap safe.
    storage& file_;
    const Link header_size_;

    // Payload size is protected by mutex.
    Link payload_size_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, if_unsigned_integer<Link> If>
#define CLASS slab_manager<Link, If>

#include <bitcoin/database/impl/slab_manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
