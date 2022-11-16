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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {
    
/// Linked list abstraction over storage for given link and record sizes.
/// if slab (Size == max_size_t), count/link is bytes otherwise records.
/// Obtaining memory object is considered const access despite the fact that
/// memory is writeable. Non-const manager access implies memory map modify.
template <typename Link, typename Key, size_t Size>
class manager
{
public:
    /// Manage byte storage device.
    manager(storage& file) NOEXCEPT;

    /// The logical record count or slab size.
    Link count() const NOEXCEPT;

    /// Reduce the number of records (false if not lesser).
    bool truncate(const Link& count) NOEXCEPT;

    /// Allocate records and return first logical position (eof possible).
    /// For record, size is number of records to allocate (link + data).
    /// For slab size must include bytes (link + data) [key is part of data].
    Link allocate(const Link& size) NOEXCEPT;

    /// Return memory object for record at specified position (null possible).
    /// Obtaining memory object is considered const access despite fact that
    /// memory is writeable. Non-const access implies memory map modify.
    /// MEMORY_PTR HOLDS SHARED LOCK ON STORAGE REMAP, DO NOT EXTEND LIFETIME.
    memory_ptr get(const Link& link) const NOEXCEPT;

    /// Return memory object for the full memory map.
    /// MEMORY_PTR HOLDS SHARED LOCK ON STORAGE REMAP, DO NOT EXTEND LIFETIME.
    memory_ptr get() const NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;
    static constexpr Link position_to_link(size_t position) NOEXCEPT;

    // Thread and remap safe.
    storage& file_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, typename Key, size_t Size>
#define CLASS manager<Link, Key, Size>

#include <bitcoin/database/impl/primitives/manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
