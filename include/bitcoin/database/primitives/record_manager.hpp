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
#ifndef LIBBITCOIN_DATABASE_RECORD_MANAGER_HPP
#define LIBBITCOIN_DATABASE_RECORD_MANAGER_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// The record manager represents a collection of fixed size chunks of
/// data referenced by an index. The file will be resized accordingly
/// and the total number of records updated so new chunks can be allocated.
/// It also provides logical record mapping to the record memory address.
template <typename LinkType>
class record_manager
  : noncopyable
{
public:
    record_manager(storage& file, size_t header_size, size_t record_size);

    /// Create record manager.
    bool create();

    /// Prepare manager for usage.
    bool start();

    /// Synchronise to disk.
    void sync();

    /// The number of records in this container.
    LinkType count() const;

    /// Change the number of records of this container (truncation).
    void set_count(const LinkType value);

    /// Allocate records and return first logical index, sync() after writing.
    LinkType new_records(LinkType count);

    /// Return memory object for the record at the specified index.
    memory_ptr get(LinkType record) const;

private:
    // The record index of a disk position.
    LinkType position_to_record(file_offset position) const;

    // The disk position of a record index.
    file_offset record_to_position(LinkType record) const;

    // Read the count of the records from the file.
    void read_count();

    // Write the count of the records from the file.
    void write_count();

    // This class is thread and remap safe.
    storage& file_;
    const size_t header_size_;
    const size_t record_size_;

    // Record count is protected by mutex.
    LinkType record_count_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_manager.ipp>

#endif
