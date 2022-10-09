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

/// Data storage interface for a single memory-mapped file.
/// The implementation must be thread safe, allowing concurent read and write.
class BCD_API storage
{
public:
    /// Map file to memory, must be unmapped, not idempotent.
    virtual bool map() NOEXCEPT = 0;

    /// Flush memory map to disk if mapped, idempotent.
    virtual bool flush() const NOEXCEPT = 0;

    /// Unmap file, must be mapped, restartable.
    virtual bool unmap() NOEXCEPT = 0;

    /// Determine if the file is mapped.
    virtual bool mapped() const NOEXCEPT = 0;

    /// The current capacity for mapped data.
    virtual size_t capacity() const NOEXCEPT = 0;

    /// The current logical size of mapped data.
    virtual size_t logical() const NOEXCEPT = 0;

    /// Get protected shared access to memory.
    virtual memory_ptr access() NOEXCEPT(false) = 0;

    /// Change logical size to specified size, return access.
    /// Increases or shrinks capacity to match logical size.
    virtual memory_ptr resize(size_t required) NOEXCEPT(false) = 0;

    /// Increase the logical size to specified size, return access.
    /// Increases capacity to at least logical size, as necessary.
    virtual memory_ptr reserve(size_t required) NOEXCEPT(false) = 0;
};

} // namespace database
} // namespace libbitcoin

#endif
