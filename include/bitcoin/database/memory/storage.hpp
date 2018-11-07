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
#ifndef LIBBITCOIN_DATABASE_STORAGE_HPP
#define LIBBITCOIN_DATABASE_STORAGE_HPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// The implementation must be thread safe, allowing concurent read and write.
class BCD_API storage
  : noncopyable
{
public:
    /// Open and map database files, must be closed.
    virtual bool open() = 0;

    /// Flush the memory map to disk, idempotent.
    virtual bool flush() const = 0;

    /// Unmap and release files, restartable, idempotent.
    virtual bool close() = 0;

    /// Determine if the database is closed.
    virtual bool closed() const = 0;

    /// The current physical (vs. logical) size of the map.
    virtual size_t size() const = 0;

    /// Get protected shared access to memory, starting at first byte.
    virtual memory_ptr access() = 0;

    /// Resize the logical map to the specified size, return access.
    /// Increase or shrink the physical size to match the logical size.
    virtual memory_ptr resize(size_t size) = 0;

    /// Resize the logical map to the specified size, return access.
    /// Increase the physical size to at least the logical size.
    virtual memory_ptr reserve(size_t size) = 0;
};

} // namespace database
} // namespace libbitcoin

#endif
