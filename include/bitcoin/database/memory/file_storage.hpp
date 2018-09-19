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
#ifndef LIBBITCOIN_DATABASE_FILE_STORAGE_HPP
#define LIBBITCOIN_DATABASE_FILE_STORAGE_HPP

#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe, allowing concurent read and write.
/// A change to the size of the memory map waits on and locks read and write.
class BCD_API file_storage
  : public storage
{
public:
    typedef boost::filesystem::path path;
    static const size_t default_expansion;

    /// Construct a database (start is currently called, may throw).
    file_storage(const path& filename);
    file_storage(const path& filename, size_t expansion);

    /// Close the database.
    ~file_storage();

    /// Open and map database files, must be closed.
    bool open();

    /// Flush the memory map to disk, idempotent.
    bool flush() const;

    /// Unmap and release files, restartable, idempotent.
    bool close();

    /// Determine if the database is closed.
    bool closed() const;

    /// The current physical (vs. logical) size of the map.
    size_t size() const;

    /// Get protected shared access to memory, starting at first byte.
    memory_ptr access();

    /// Throws runtime_error if insufficient space.
    /// Resize the logical map to the specified size, return access.
    /// Increase or shrink the physical size to match the logical size.
    memory_ptr resize(size_t size);

    /// Throws runtime_error if insufficient space.
    /// Resize the logical map to the specified size, return access.
    /// Increase the physical size to at least the logical size.
    memory_ptr reserve(size_t size);

private:
    static size_t file_size(int file_handle);
    static int open_file(const boost::filesystem::path& filename);
    static bool handle_error(const std::string& context,
        const boost::filesystem::path& filename);

    size_t page() const;
    bool unmap();
    bool map(size_t size);
    bool remap(size_t size);
    bool truncate(size_t size);
    bool truncate_mapped(size_t size);
    bool validate(size_t size);
    memory_ptr reserve(size_t size, size_t growth_ratio);

    void log_mapping() const;
    void log_resizing(size_t size) const;
    void log_flushed() const;
    void log_unmapping() const;
    void log_unmapped() const;

    // File system.
    const int file_handle_;
    const size_t expansion_;
    const boost::filesystem::path filename_;

    // Protected by mutex.
    bool closed_;
    uint8_t* data_;
    size_t file_size_;
    size_t logical_size_;
    mutable upgrade_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
