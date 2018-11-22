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
#ifndef LIBBITCOIN_DATABASE_RECORD_MANAGER_HPP
#define LIBBITCOIN_DATABASE_RECORD_MANAGER_HPP

#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// The record manager represents a collection of fixed size chunks of
/// data referenced by an index. The file will be resized accordingly
/// and the total number of records updated so new chunks can be allocated.
/// It also provides logical record mapping to the record memory address.
template <typename Link>
class record_manager
  : system::noncopyable
{
public:
    // This cast is a VC++ workaround is OK because Link must be unsigned.
    //static constexpr Link empty = std::numeric_limits<Link>::max();
    static const Link not_allocated = (Link)bc::max_uint64;

    record_manager(storage& file, size_t header_size, size_t record_size);

    /// Create record manager.
    bool create();

    /// Prepare manager for usage.
    bool start();

    /// Commit record count to the file.
    void commit();

    /// The number of records in this container.
    Link count() const;

    /// Change the number of records of this container (truncation).
    void set_count(Link value);

    /// Allocate records and return first logical index, commit after writing.
    Link allocate(size_t count);

    /// Return memory object for the record at the specified index.
    memory_ptr get(Link link) const;

private:
    // The record index of a disk position.
    Link position_to_link(file_offset position) const;

    // The disk position of a record index.
    file_offset link_to_position(Link link) const;

    // Read the count of the records from the file.
    void read_count();

    // Write the count of the records from the file.
    void write_count();

    // This class is thread and remap safe.
    storage& file_;
    const size_t header_size_;
    const size_t record_size_;

    // Record count is protected by mutex.
    Link record_count_;
    mutable system::shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_manager.ipp>

#endif
