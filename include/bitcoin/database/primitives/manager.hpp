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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {
    
/// Linked list abstraction over storage for given link and record sizes.
template <typename Link, size_t Size,
    if_link<Link> = true>
class manager
{
public:
    /// Expose for public extraction.
    using link = Link;

    /// End of file sentinel.
    static constexpr auto eof = system::bit_all<Link>;

    /// Manage byte storage device.
    manager(storage& file) NOEXCEPT;

    /// The logical record count.
    Link size() const NOEXCEPT;

    /// Reduce the number of records (false if not lesser).
    bool truncate(Link count) NOEXCEPT;

    /// Allocate records and return first logical position (eof possible).
    Link allocate(Link count) NOEXCEPT;

    /// Return memory object for record at specified position (null possible).
    memory_ptr get(Link link) const NOEXCEPT;

private:
    // Slabs don't use record offsetting (size_ one eliminates conversion).
    static constexpr auto size_ = is_zero(Size) ? one : sizeof(Link) + Size;

    static constexpr Link position_to_link(size_t position) NOEXCEPT
    {
        BC_ASSERT(system::is_multiple(position, size_));
        return system::possible_narrow_cast<Link>(position / size_);
    }

    static constexpr size_t link_to_position(Link link) NOEXCEPT
    {
        return link * size_;
    }

    // Thread and remap safe.
    storage& file_;
};

template <typename Link, size_t Size>
using record_manager = manager<Link, Size>;

// Zero byte "record" implies slab manager.
template <typename Link>
using slab_manager = manager<Link, zero>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, size_t Size, if_link<Link> If>
#define CLASS manager<Link, Size, If>

#include <bitcoin/database/impl/primitives/manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
