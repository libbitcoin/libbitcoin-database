/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_INTERFACES_STORAGE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_INTERFACES_STORAGE_HPP

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {

/// Mapped memory interface.
class storage
{
public:
    static constexpr auto eof = system::bit_all<size_t>;

    /// Open file, must be closed.
    virtual code open() NOEXCEPT = 0;

    /// Close file, must be unloaded, idempotent.
    virtual code close() NOEXCEPT = 0;

    /// Map file to memory, must be open and unloaded.
    virtual code load() NOEXCEPT = 0;

    /// Clear disk full condition, fails if fault, must be loaded, idempotent.
    virtual code reload() NOEXCEPT = 0;

    /// Flush memory map to disk, suspend writes for call, must be loaded.
    virtual code flush() NOEXCEPT = 0;

    /// Flush, unmap and truncate to logical, restartable, idempotent.
    virtual code unload() NOEXCEPT = 0;

    /// The filesystem path of the backing storage.
    virtual const std::filesystem::path& file() const NOEXCEPT = 0;

    /// The current logical size of the memory map (zero if closed).
    virtual size_t size() const NOEXCEPT = 0;

    /// The current capacity of the memory map (zero if unmapped).
    virtual size_t capacity() const NOEXCEPT = 0;

    /// Reduce logical size to specified (false if size exceeds logical).
    virtual bool truncate(size_t size) NOEXCEPT = 0;

    /// Increase logical size to specified as required (false only if fails).
    virtual bool expand(size_t size) NOEXCEPT = 0;

    /// Reserve additional bytes to guard against disk full (false if fails).
    virtual bool reserve(size_t size) NOEXCEPT = 0;

    /// Increase logical bytes and return offset to first allocated (or eof).
    virtual size_t allocate(size_t chunk) NOEXCEPT = 0;

    /// Get remap-protected r/w access to offset (or null) allocated to size.
    virtual memory_ptr set(size_t offset, size_t size,
        uint8_t backfill) NOEXCEPT = 0;

    /// Get remap-protected r/w access to start/offset of memory map (or null).
    virtual memory_ptr get(size_t offset=zero) const NOEXCEPT = 0;

    /// Get unprotected r/w access to start/offset of memory map (or null).
    virtual memory::iterator get_raw(size_t offset=zero) const NOEXCEPT = 0;

    /// Get the fault condition.
    virtual code get_fault() const NOEXCEPT = 0;

    /// Get the space required to clear the disk full condition.
    virtual size_t get_space() const NOEXCEPT = 0;
};

} // namespace database
} // namespace libbitcoin

#endif
