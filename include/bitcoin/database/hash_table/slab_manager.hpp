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
#ifndef LIBBITCOIN_DATABASE_SLAB_MANAGER_HPP
#define LIBBITCOIN_DATABASE_SLAB_MANAGER_HPP

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

typedef memory_array<array_index, file_offset> htdb_slab_header;

BC_CONSTEXPR size_t min_slab_fsize = sizeof(file_offset);
BC_CONSTFUNC size_t htdb_slab_header_fsize(size_t buckets)
{
    return sizeof(file_offset) + min_slab_fsize * buckets;
}

/// The slab manager represents a growing collection of various sized
/// slabs of data on disk. It will resize the file accordingly and keep
/// track of the current end pointer so new slabs can be allocated.
/// It also provides logical slab mapping to the slab memory address.
class BCD_API slab_manager
{
public:
    slab_manager(memory_map& file, file_offset sector_start);

    /// Create slab allocator.
    void create();

    /// Prepare manager for use.
    void start();

    /// Synchronise slab allocator to disk.
    void sync();

    /// Allocate a slab.
    /// Call sync() after writing the record.
    file_offset new_slab(size_t bytes_needed);

    /// Return a slab from its byte-wise position relative to start.
    uint8_t* get0(file_offset position);

    /// Return a const slab memory address from its file offest.
    const uint8_t* get0(file_offset position) const;

private:

    // Ensure bytes for a new record are available.
    void reserve(size_t bytes_needed);

    // Read the size of the data from the file.
    void read_size();

    // Write the size of the data from the file.
    void write_size();

    memory_map& file_;
    const file_offset start_;
    std::atomic<file_offset> size_;
    mutable boost::shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
