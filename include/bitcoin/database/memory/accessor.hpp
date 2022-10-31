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
#ifndef LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_HPP
#define LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_HPP

#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {

/// Shared r/w access to a memory buffer, mutex prevents memory remap.
template <typename Mutex>
class accessor final
  : public memory
{
public:
    /// Mutex guards against remap while object in scope.
    accessor(Mutex& mutex) NOEXCEPT;

    /// Set the buffer.
    void assign(uint8_t* begin, uint8_t* end) NOEXCEPT;

    /// Increment the pointer the specified number of bytes within the record.
    void increment(size_t value) NOEXCEPT override;

    /// Get buffer (guarded against remap only).
    uint8_t* begin() NOEXCEPT override;
    const uint8_t* end() const NOEXCEPT override;

    /// Cast to r/w slices for use in stream construction.
    operator system::data_slab() NOEXCEPT;
    operator system::data_reference() const NOEXCEPT;

private:
    uint8_t* begin_{};
    uint8_t* end_{};
    std::shared_lock<Mutex> shared_lock_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Mutex>
#define CLASS accessor<Mutex>

#include <bitcoin/database/impl/memory/accessor.ipp>

#undef CLASS
#undef TEMPLATE

#endif
