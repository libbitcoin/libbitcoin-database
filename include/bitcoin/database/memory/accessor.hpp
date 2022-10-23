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
#ifndef LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_HPP
#define LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_HPP

#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// Shared access to a memory buffer, mutex prevents memory remap.
/// Caller must know the buffer size as it is unguarded/unmanaged.
template <typename Mutex>
class accessor final
  : public memory
{
public:
    /// Mutex guards against remap while object is in scope.
    accessor(Mutex& mutex) NOEXCEPT;

    /// Set the buffer pointer.
    void assign(uint8_t* data) NOEXCEPT;

    /// Get the buffer pointer (unguarded except for remap).
    uint8_t* data() NOEXCEPT override;

    /// Advance the buffer pointer a specified number of bytes.
    void increment(size_t size) NOEXCEPT override;

private:
    uint8_t* data_;
    std::shared_lock<Mutex> shared_lock_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/memory/accessor.ipp>

#endif
