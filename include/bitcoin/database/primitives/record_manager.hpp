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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_MANAGER_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_MANAGER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

// Manager provides array abstraction over storage.
template <typename Storage, typename Link, size_t Size,
    if_base_of<database::storage, Storage> = true,
    if_unsigned_integer<Link> = true>
class record_manager
{
public:
    static constexpr auto eof = system::bit_all<Link>;

    /// Open (create as necessary) the file.
    record_manager(Storage& file) NOEXCEPT;

    /// The logical record count, thread safe (eof if invalid).
    Link size() const NOEXCEPT;

    /// Reduce the logical record count, thread safe (false if not lesser).
    bool truncate(Link count) NOEXCEPT;

    /// Allocate records and return first logical position (eof possible).
    Link allocate(size_t count) NOEXCEPT;

    /// Return memory object for record at specified position (null possible).
    memory_ptr get(Link link) const NOEXCEPT;

private:
    static constexpr Link position_to_link(size_t position) NOEXCEPT
    {
        return position / Size;
    }

    static constexpr size_t link_to_position(Link link) NOEXCEPT
    {
        return link * Size;
    }

    // This class is thread and remap safe.
    Storage& file_;

    // Record count is protected by mutex.
    Link count_;
    mutable shared_mutex mutex_;
};

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Storage, typename Link, size_t Size, \
if_base_of<database::storage, Storage> If1, if_unsigned_integer<Link> If2>
#define CLASS record_manager<Storage, Link, Size, If1, If2>

#include <bitcoin/database/impl/primitives/record_manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
