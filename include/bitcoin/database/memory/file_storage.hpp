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
#ifndef LIBBITCOIN_DATABASE_MEMORY_FILE_STORAGE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_FILE_STORAGE_HPP

#include <string>
#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe, allowing concurent read and write.
/// A change to memory map size waits on and blocks read/write.
class BCD_API file_storage final
  : public storage
{
public:
    DELETE4(file_storage);

    typedef std::filesystem::path path;
    static const size_t default_expansion;
    static const uint64_t default_capacity;

    /// Open database file.
    file_storage(const path& filename) NOEXCEPT;
    file_storage(const path& filename, size_t minimum,
        size_t expansion) NOEXCEPT;

    /// Close the database file.
    ~file_storage() NOEXCEPT;

    /// Map file to memory, must be unmapped, not idempotent.
    bool map() NOEXCEPT override;

    /// Flush memory map to disk if mapped, idempotent.
    bool flush() const NOEXCEPT override;

    /// Unmap file, must be mapped, restartable.
    bool unmap() NOEXCEPT override;

    /// Determine if the file is mapped.
    bool mapped() const NOEXCEPT override;

    /// The current capacity for mapped data.
    size_t capacity() const NOEXCEPT override;

    /// The current logical size of mapped data.
    size_t logical() const NOEXCEPT override;

    /// Get protected shared access to memory.
    memory_ptr access() NOEXCEPT(false) override;

    /// Throws runtime_error if insufficient space.
    /// Resize the logical map to the specified size, return access.
    /// Increase or shrink the physical size to match the logical size.
    memory_ptr resize(size_t required) NOEXCEPT(false) override;

    /// Throws runtime_error if insufficient space.
    /// Resize the logical map to the specified size, return access.
    /// Increase the physical size to at least the logical size.
    memory_ptr reserve(size_t required) NOEXCEPT(false) override;

private:
    static size_t file_size(int file_handle) NOEXCEPT;
    static int close_file(int file_handle) NOEXCEPT;
    static int open_file(const std::filesystem::path& filename) NOEXCEPT;
    static bool handle_error(const std::string& context,
        const std::filesystem::path& filename) NOEXCEPT;

    size_t page() const NOEXCEPT;
    bool unmap_() NOEXCEPT;
    bool map(size_t size) NOEXCEPT;
    bool remap(size_t size) NOEXCEPT;
    bool truncate(size_t size) NOEXCEPT;
    bool truncate_mapped(size_t size) NOEXCEPT;
    bool validate(size_t size) NOEXCEPT;
    memory_ptr reserve(size_t required, size_t minimum,
        size_t expansion) NOEXCEPT(false);

    void log_mapping() const NOEXCEPT;
    void log_resizing(size_t size) const NOEXCEPT;
    void log_flushed() const NOEXCEPT;
    void log_unmapping() const NOEXCEPT;
    void log_unmapped() const NOEXCEPT;

    // File system.
    const int file_handle_;
    const size_t minimum_;
    const size_t expansion_;
    const std::filesystem::path filename_;

    // Protected by mutex.
    bool mapped_;
    uint8_t* data_;
    size_t capacity_;
    size_t logical_size_;
    mutable upgrade_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
