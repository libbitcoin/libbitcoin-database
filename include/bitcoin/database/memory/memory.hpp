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
#ifndef LIBBITCOIN_DATABASE_MEMORY_HPP
#define LIBBITCOIN_DATABASE_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

#ifdef REMAP_SAFETY

/// This class provides remap safe access to file-mapped memory.
/// The memory size is unprotected and unmanaged.
class BCD_API memory
{
public:
    typedef std::shared_ptr<memory> ptr;

    memory(uint8_t* data, shared_mutex& mutex);
    ~memory();

    /// This class is not copyable.
    memory(const memory& other) = delete;

    /// Get the address indicated by the pointer.
    uint8_t* buffer();

    /// Increment the pointer the specified number of bytes within the record.
    void increment(size_t value);

private:
    uint8_t* data_;
    shared_mutex& mutex_;
    shared_lock lock_;
};

#endif // REMAP_SAFETY

#ifdef REMAP_SAFETY
    typedef memory::ptr memory_ptr;
    #define REMAP_ADDRESS(pointer) pointer->buffer()
    #define REMAP_INCREMENT(pointer, offset) pointer->increment(offset)
    #define REMAP_SAFE(data, mutex) std::make_shared<memory>(data_, mutex_)
    #define REMAP_READ(mutex) shared_lock lock(mutex)
    #define REMAP_WRITE(mutex) unique_lock lock(mutex)
#else
    typedef uint8_t* memory_ptr;
    #define REMAP_ADDRESS(pointer) pointer
    #define REMAP_INCREMENT(pointer, offset) pointer += (offset)
    #define REMAP_SAFE(data, mutex) data
    #define REMAP_READ(mutex)
    #define REMAP_WRITE(mutex)
#endif // REMAP_SAFETY

#ifdef ALLOCATE_SAFETY
    #define ALLOCATE_READ(mutex) shared_lock lock(mutex)
    #define ALLOCATE_WRITE(mutex) unique_lock lock(mutex)
#else
    #define ALLOCATE_READ(mutex)
    #define ALLOCATE_WRITE(mutex)
#endif // ALLOCATE_SAFETY

} // namespace database
} // namespace libbitcoin

#endif
