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
#ifndef LIBBITCOIN_DATABASE_MEMORY_STORAGE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_STORAGE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// Mapped memory abstraction of a file.
class BCD_API storage
{
public:
    /// Close file, idempotent.
    virtual bool close() NOEXCEPT = 0;

    /// Map file to memory, must be unmapped.
    virtual bool load_map() NOEXCEPT = 0;

    /// Flush logical size of memory map to disk, must be mapped.
    virtual bool flush_map() const NOEXCEPT = 0;

    /// Flush, unmap and truncate to logical, restartable, idempotent.
    virtual bool unload_map() NOEXCEPT = 0;

    /// True if the file is mapped.
    virtual bool is_mapped() const NOEXCEPT = 0;

    /// True if the file is closed (or failed to open).
    virtual bool is_closed() const NOEXCEPT = 0;

    /// The current size of the persistent file (zero if closed).
    virtual size_t size() const NOEXCEPT = 0;

    /// The current logical size of the memory map (zero if closed).
    virtual size_t logical() const NOEXCEPT = 0;

    /// The current capacity of the memory map (zero if unmapped).
    virtual size_t capacity() const NOEXCEPT = 0;

    /// Get protected read/write access to start of memory map.
    virtual memory_ptr get() THROWS = 0;

    /// Change logical size to the specified total size, return access.
    /// Increases or shrinks the capacity/file size to match required size.
    virtual memory_ptr resize(size_t required) THROWS = 0;

    /// Increase logical size to the specified total size, return access.
    /// Increases the capacity/file size to at least the required size.
    virtual memory_ptr reserve(size_t required) THROWS = 0;
};

} // namespace database
} // namespace libbitcoin

#endif
