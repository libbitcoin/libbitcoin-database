/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_RECORD_MANAGER_HPP
#define LIBBITCOIN_DATABASE_RECORD_MANAGER_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <boost/thread.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory_array.hpp>
#include <bitcoin/database/memory/memory_map.hpp>

namespace libbitcoin {
namespace database {

typedef memory_array<array_index, array_index> htdb_record_header;

BC_CONSTEXPR size_t min_records_fsize = sizeof(array_index);
BC_CONSTFUNC size_t htdb_record_header_fsize(size_t buckets)
{
    return sizeof(array_index) + min_records_fsize * buckets;
}

/// The record manager represents a collection of fixed size chunks of
/// data referenced by an index. The file will be resized accordingly
/// and the total number of records updated so new chunks can be allocated.
/// It also provides logical record mapping to the record memory address.
class BCD_API record_manager
{
public:
    record_manager(memory_map& file, file_offset sector_start,
        size_t record_size);

    /// Create record manager.
    void create();

    /// Prepare manager for usage.
    void start();

    /// Synchronise to disk.
    void sync();

    /// The number of records in this container.
    array_index count() const;

    /// Change the number of records of this container.
    void set_count(const array_index value);

    /// Allocate a record and return its logical index.
    /// Call sync() after writing the record.
    array_index new_record(/* size_t records=1 */);

    /// Return a record from its logical index.
    uint8_t* get0(array_index record);

    /// Return a const record memory address from its logical index.
    const uint8_t* get0(array_index record) const;

private:

    // Ensure bytes for a new record are available.
    void reserve(size_t count);

    //// The record index of a disk position.
    ////array_index position_to_record(file_offset position) const;

    // The disk position of a record index.
    file_offset record_to_position(array_index record) const;

    // Read the count of the records from the file.
    void read_count();

    // Write the count of the records from the file.
    void write_count();

    memory_map& file_;
    const file_offset start_;
    std::atomic<array_index> count_;
    mutable boost::shared_mutex mutex_;
    const size_t record_size_;
};

} // namespace database
} // namespace libbitcoin

#endif
