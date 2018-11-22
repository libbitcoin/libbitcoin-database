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
#ifndef LIBBITCOIN_DATABASE_ACCESSOR_HPP
#define LIBBITCOIN_DATABASE_ACCESSOR_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// This class provides shared/protected read/write access to a memory buffer.
/// The call caller must know the buffer size as it is unprotected/unmanaged.
class BCD_API accessor
  : public memory, system::noncopyable
{
public:
    /// Assign a null upgradeable buffer pointer.
    accessor(system::upgrade_mutex& mutex);

    /// Free the buffer pointer lock.
    ~accessor();

    /// Get the buffer pointer.
    uint8_t* buffer();

    /// Set the buffer pointer and lock for shared access.
    void assign(uint8_t* data);

    /// Advance the buffer pointer a specified number of bytes.
    void increment(size_t value);

private:
    system::upgrade_mutex& mutex_;
    uint8_t* data_;
};

} // namespace database
} // namespace libbitcoin

#endif
