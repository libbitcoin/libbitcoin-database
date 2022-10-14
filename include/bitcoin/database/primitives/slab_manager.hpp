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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_SLAB_MANAGER_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_SLAB_MANAGER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

template <typename Storage, typename Link,
    if_base_of<database::storage, Storage> = true,
    if_unsigned_integer<Link> = true>
class slab_manager
{
public:
    static constexpr auto eof = system::bit_all<Link>;

    /// Open (create as necessary) the file.
    slab_manager(Storage& file) NOEXCEPT;

    /// The logical file size, thread safe (eof if invalid).
    Link size() const NOEXCEPT;

    /// Allocate an appended slab and return its position (eof possible).
    Link allocate(size_t size) NOEXCEPT;

    /// Return memory object for slab at specified position (null possible).
    memory_ptr get(Link position) const NOEXCEPT;

private:
    // This class is thread and remap safe.
    Storage& file_;

    // Payload size is protected by mutex.
    Link size_;
    mutable shared_mutex mutex_;
};

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Storage, typename Link, \
if_base_of<database::storage, Storage> If1, if_unsigned_integer<Link> If2>
#define CLASS slab_manager<Storage, Link, If1, If2>

#include <bitcoin/database/impl/primitives/slab_manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
