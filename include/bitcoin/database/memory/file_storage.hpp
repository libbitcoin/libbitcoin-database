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

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

/// Thread safe access to a memory-mapped file.
class BCD_API file_storage final
  : public storage
{
public:
    DELETE4(file_storage);

    file_storage(const std::filesystem::path& filename, size_t minimum=1,
        size_t expansion=50) NOEXCEPT;

    /// File should be explicitly unmapped before destruct.
    ~file_storage() NOEXCEPT;

    /// Open file, must be closed.
    bool open() NOEXCEPT override;

    /// Close file, must be unmapped, idempotent.
    bool close() NOEXCEPT override;

    /// True if the file is closed (or failed to open).
    bool is_open() const NOEXCEPT override;

    /// Map file to memory, must be unmapped.
    bool load_map() NOEXCEPT override;

    /// Flush logical size of memory map to disk, must be mapped.
    /// Requires exclusive access to memory map.
    bool flush_map() const NOEXCEPT override;

    /// Flush, unmap and truncate to logical, restartable, idempotent.
    bool unload_map() NOEXCEPT override;

    /// True if the file is mapped.
    bool is_mapped() const NOEXCEPT override;

    /// The current size of the persistent file (zero if closed).
    size_t size() const NOEXCEPT override;

    /// The current logical size of the memory map (zero if closed).
    size_t logical() const NOEXCEPT override;

    /// The current capacity of the memory map (zero if unmapped).
    size_t capacity() const NOEXCEPT override;

    /// Get protected read/write access to start of memory map.
    memory_ptr get() NOEXCEPT override;

    /// Returns nullptr on failure (unmapped or disk full).
    /// Change logical size to the specified total size, return access.
    /// Increases or shrinks the capacity/file size to match required size.
    memory_ptr resize(size_t required) NOEXCEPT override;

    /// Returns nullptr on failure (unmapped or disk full).
    /// Increase logical size to the specified total size, return access.
    /// Increases the capacity/file size to at least the required size.
    memory_ptr reserve(size_t required) NOEXCEPT override;

protected:
    static constexpr size_t get_resize(size_t required, size_t minimum,
        size_t expansion) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_STATIC_CAST)
        const auto resize = required * ((expansion + 100.0) / 100.0);
        const auto target = std::max(minimum, static_cast<size_t>(resize));
        BC_POP_WARNING()

        BC_ASSERT_MSG(target >= required, "unexpected truncation");
        return target;
    }

private:
    using path = std::filesystem::path;

    // Mapping utilities.
    bool flush() const NOEXCEPT;
    bool unmap() NOEXCEPT;
    bool map() NOEXCEPT;
    bool remap(size_t size) NOEXCEPT;
    bool finalize(size_t size) NOEXCEPT;
    memory_ptr reserve(size_t required, size_t minimum,
        size_t expansion) NOEXCEPT;

    // Constants.
    const std::filesystem::path filename_;
    const size_t minimum_;
    const size_t expansion_;

    // Protected by mutex.
    uint8_t* map_;
    mutable std::shared_mutex map_mutex_;

    // Protected by mutex.
    bool mapped_;
    size_t logical_;
    size_t capacity_;
    int file_descriptor_;
    mutable boost::upgrade_mutex field_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
