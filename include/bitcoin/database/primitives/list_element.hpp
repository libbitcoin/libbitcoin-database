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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_LIST_ELEMENT_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_LIST_ELEMENT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// A hash table key-conflict row, implemented as a linked list.
/// Link cannot exceed 64 bits. A default Key creates an unkeyed list.
template <typename Manager, typename Link, typename Key,
    if_unsigned_integer<Link> = true,
    if_integral_array<Key> = true>
class list_element
{
public:
    static constexpr auto not_found = system::possible_narrow_cast<Link>(
        max_uint64);

    /// The stored size of a value with the given size.
    static constexpr size_t size(size_t value_size) NOEXCEPT
    {
        return array_count<Key> + sizeof(Link) + value_size;
    }

    /// Construct for a new element.
    list_element(Manager& manager, shared_mutex& mutex) NOEXCEPT;

    /// Construct for an existing element.
    list_element(Manager& manager, Link link, shared_mutex& mutex) NOEXCEPT;

    /// Allocate and populate a new unkeyed record element.
    Link create(auto& write) NOEXCEPT;

    /// Allocate and populate a new keyed record element.
    Link create(const Key& key, auto& write) NOEXCEPT;

    /// Allocate and populate a new keyed slab element.
    Link create(const Key& key, auto& write, size_t value_size) NOEXCEPT;

    /// Update this element to the next element (read next from file).
    bool jump_next() NOEXCEPT;

    /// Convert the instance into a terminator.
    void terminate() NOEXCEPT;

    /// Connect the next element (write to file).
    void set_next(Link next) const NOEXCEPT;

    /// Write to the state of the element (write to file).
    void write(auto& write) const NOEXCEPT;

    /// Read from the state of the element.
    void read(auto& read) const NOEXCEPT;

    /// True if the element key (read from file) matches the parameter.
    bool match(const Key& key) const NOEXCEPT;

    /// The key of this element (read from file).
    Key key() const NOEXCEPT;

    /// The address of this element.
    Link link() const NOEXCEPT;

    /// The address of the next element (read from file).
    Link next() const NOEXCEPT;

    /// A list terminator for this instance.
    list_element terminator() const NOEXCEPT;

    /// The element is terminal (not found, cannot be read).
    bool terminal() const NOEXCEPT;

    /// Cast operator, true if element was found (not terminal).
    operator bool() const NOEXCEPT;

    /// Equality comparison operators, compares link value only.
    bool operator==(list_element other) const NOEXCEPT;
    bool operator!=(list_element other) const NOEXCEPT;

private:
    static constexpr auto link_size = sizeof(Link);
    static constexpr auto key_size = size_of<Key>();

    memory_ptr data(size_t bytes) const NOEXCEPT;
    void initialize(const Key& key, auto& write) NOEXCEPT;

    Link link_;
    Manager& manager_;
    shared_mutex& mutex_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Manager, typename Link, typename Key,\
if_unsigned_integer<Link> If1, if_integral_array<Key> If2>
#define CLASS list_element<Manager, Link, Key, If1, If2>

#include <bitcoin/database/impl/primitives/list_element.ipp>

#undef CLASS
#undef TEMPLATE

#endif
