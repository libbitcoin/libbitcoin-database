/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/primitives/keys.hpp>

namespace libbitcoin {
namespace database {
    
/// Linked list abstraction over storage for given link and record sizes.
/// if slab (Size == max_size_t), count/link is bytes, otherwise records.
/// Obtaining memory object is considered const access despite the fact that
/// memory is writeable. Non-const manager access implies memory map modify.
template <class Link, class Key, size_t Size>
class manager
{
public:
    using integer = typename Link::integer;
    static constexpr Link position_to_link(size_t position) NOEXCEPT;
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;
    static constexpr integer cast_link(size_t link) NOEXCEPT;

    DEFAULT_COPY_MOVE_DESTRUCT(manager);

    /// Manage byte storage device.
    manager(storage& file) NOEXCEPT;

    /// The file size.
    inline size_t size() const NOEXCEPT;

    /// The logical record count.
    inline Link count() const NOEXCEPT;

    /// The reserved byte count.
    inline size_t capacity() const NOEXCEPT;

    /// Reduce logical size to specified records (false if exceeds logical).
    bool truncate(const Link& count) NOEXCEPT;

    /// Increase logical size to specified bytes as required (false if fails).
    bool expand(const Link& count) NOEXCEPT;

    /// Increase capacity by specified bytes (false only if fails).
    bool reserve(const Link& count) NOEXCEPT;

    /// Increase logical by specified bytes, return offset to first (or eof).
    /// For record, count is number of records to allocate (link + data).
    /// For slab count must include bytes (link + data) [key is part of data].
    Link allocate(const Link& count) NOEXCEPT;

    /// Return memory object for record at specified position (null possible).
    /// Obtaining memory object is considered const access despite fact that
    /// memory is writeable. Non-const access implies memory map modify.
    inline memory_ptr get(const Link& link) const NOEXCEPT;

    /// Return memory object for full memory map (null only if oom or unloaded).
    inline memory_ptr get() const NOEXCEPT;

    /// Get the fault condition.
    code get_fault() const NOEXCEPT;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Resume from disk full condition.
    code reload() NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    static constexpr auto key_size = keys::size<Key>();

    // Thread and remap safe.
    storage& file_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, class Key, size_t Size>
#define CLASS manager<Link, Key, Size>

#include <bitcoin/database/impl/primitives/manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
