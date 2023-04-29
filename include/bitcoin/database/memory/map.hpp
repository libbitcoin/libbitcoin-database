/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MAP_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MAP_HPP

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>

namespace libbitcoin {
namespace database {

/// Thread safe access to a memory-mapped file.
class BCD_API map
  : public storage
{
public:
    DELETE_COPY_MOVE(map);

    map(const std::filesystem::path& filename, size_t minimum=1,
        size_t expansion=0) NOEXCEPT;

    /// Destruct for debug assertion only.
    virtual ~map() NOEXCEPT;

    /// True if the file is closed (or failed to open).
    bool is_open() const NOEXCEPT;

    /// True if the memory map is loaded.
    bool is_loaded() const NOEXCEPT;

    /// storage interface
    /// -----------------------------------------------------------------------

    /// Open file, must be closed.
    code open() NOEXCEPT override;

    /// Close file, must be unloaded, idempotent.
    code close() NOEXCEPT override;

    /// Map file to memory, must be loaded.
    code load() NOEXCEPT override;

    /// Flush memory map to disk, suspend writes for call, must be loaded.
    code flush() const NOEXCEPT override;

    /// Flush, unmap and truncate to logical, restartable, idempotent.
    code unload() NOEXCEPT override;

    /// The filesystem path of the file.
    const std::filesystem::path& file() const NOEXCEPT override;

    /// The current capacity of the memory map (zero if unloaded).
    size_t capacity() const NOEXCEPT override;

    /// The current logical size of the memory map (zero if closed).
    INLINE size_t size() const NOEXCEPT override;

    /// Reduce logical size to specified (false if size exceeds logical).
    bool truncate(size_t size) NOEXCEPT override;

    /// Allocate bytes and return offset to first allocated (or eof).
    size_t allocate(size_t chunk) NOEXCEPT override;

    /// Get r/w access to start/offset of memory map (or null).
    INLINE memory_ptr get(size_t offset=zero) const NOEXCEPT override;

protected:
    size_t to_capacity(size_t required) const NOEXCEPT
    {
        BC_PUSH_WARNING(NO_STATIC_CAST)
        const auto resize = required * ((expansion_ + 100.0) / 100.0);
        const auto target = std::max(minimum_, static_cast<size_t>(resize));
        BC_POP_WARNING()
    
        BC_ASSERT_MSG(target >= required, "unexpected truncation");
        return target;
    }

private:
    using mutex = boost::upgrade_mutex;
    using path = std::filesystem::path;

    // Mapping utilities.
    bool flush_() const NOEXCEPT;
    bool unmap_() NOEXCEPT;
    bool map_() NOEXCEPT;
    bool remap_(size_t size) NOEXCEPT;
    bool finalize_(size_t size) NOEXCEPT;

    // Constants.
    const std::filesystem::path filename_;
    const size_t minimum_;
    const size_t expansion_;

    // Protected by mutex.
    uint8_t* memory_map_;
    mutable mutex map_mutex_;

    // Protected by mutex.
    bool loaded_;
    size_t logical_;
    size_t capacity_;
    int descriptor_;
    mutable mutex field_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
