/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_HPP
#define LIBBITCOIN_DATABASE_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <boost/thread.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This interface defines remap safe unrestricted access to a memory map.
class BCD_API memory
{
public:
    typedef std::shared_ptr<memory> ptr;

    /// Get the address indicated by the pointer.
    virtual uint8_t* buffer() = 0;

    /// Increment the pointer the specified number of bytes within the record.
    virtual void increment(size_t value) = 0;
};

typedef memory::ptr memory_ptr;
#define REMAP_ADDRESS(ptr) ptr->buffer()
#define REMAP_ASSIGN(ptr, data) ptr->assign(data)
#define REMAP_INCREMENT(ptr, offset) ptr->increment(offset)
#define REMAP_ACCESSOR(ptr, mutex) std::make_shared<accessor>(mutex, ptr)
#define REMAP_ALLOCATOR(mutex) std::make_shared<allocator>(mutex)
#define REMAP_READ(mutex) shared_lock lock(mutex)
#define REMAP_WRITE(mutex) unique_lock lock(mutex)

} // namespace database
} // namespace libbitcoin

#endif
