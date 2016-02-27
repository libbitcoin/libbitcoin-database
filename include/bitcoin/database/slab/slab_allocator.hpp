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
#ifndef LIBBITCOIN_DATABASE_SLAB_ALLOCATOR_HPP
#define LIBBITCOIN_DATABASE_SLAB_ALLOCATOR_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <boost/thread.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/disk/disk_array.hpp>
#include <bitcoin/database/disk/mmfile.hpp>

namespace libbitcoin {
namespace database {

typedef uint8_t* slab_byte_pointer;
typedef disk_array<array_index, file_offset> htdb_slab_header;

BC_CONSTEXPR size_t min_slab_fsize = sizeof(file_offset);
BC_CONSTFUNC size_t htdb_slab_header_fsize(size_t buckets)
{
    return sizeof(file_offset) + min_slab_fsize * buckets;
}

/**
 * The slab allocator represents a growing collection of various sized
 * slabs of data on disk. It will resize the file accordingly and keep
 * track of the current end pointer so new slabs can be allocated.
 */
class BCD_API slab_allocator
{
public:
    slab_allocator(mmfile& file, file_offset sector_start);

    /**
      * Create slab allocator.
      */
    void create();

    /**
     * Prepare allocator for usage.
     */
    void start();

    /**
     * Allocate a slab.
     * Call sync() after writing the record.
     */
    file_offset new_slab(size_t bytes_needed);

    /**
     * Synchronise slab allocator to disk.
     */
    void sync();

    /**
     * Return a slab from its byte-wise position relative to start.
     */
    const slab_byte_pointer get_slab(file_offset position) const;

    /**
     * Return distance from slab to eof providing a read boundary.
     */
    uint64_t to_eof(slab_byte_pointer slab) const;

private:

    // Ensure bytes for a new record are available.
    void reserve(size_t bytes_needed);

    // Read the size of the data from the file.
    void read_size();

    // Write the size of the data from the file.
    void write_size();

    mmfile& file_;
    const file_offset start_;
    std::atomic<file_offset> size_;
    mutable boost::shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
