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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

template <typename Type>
using if_key = if_integral_array<Type>;
template <typename Type>
using if_link = if_unsigned_integral_integer<Type>;

/// A hash table conflict linked list.
template <typename Manager, typename Key, typename Link,
    if_key<Key> = true, if_link<Link> = true>
class element
{
public:
    static constexpr auto eof = system::bit_all<Link>;

    /// Construct a terminator element.
    element(Manager& manager) NOEXCEPT;

    /// Construct for an existing element.
    element(Manager& manager, Link link) NOEXCEPT;

    /// Append a new element to the memory map.
    /// Next link is obtained from the indexing header.
    /// Returned link is used to update the indexing header.
    Link append_unkeyed_record(Link next, auto& write) NOEXCEPT;
    Link append_keyed_record(Link next, const Key& key, auto& write) NOEXCEPT;
    Link append_unkeyed_slab(Link next, auto& write, size_t limit) NOEXCEPT;
    Link append_keyed_slab(Link next, const Key& key, auto& write,
        size_t limit) NOEXCEPT;

    /// Navigation.
    bool advance() NOEXCEPT;
    bool match(const Key& key) const NOEXCEPT;
    void read_record(auto& read) const NOEXCEPT;
    void read_slab(auto& read, size_t limit) const NOEXCEPT;

    /// Properties.
    Key key() const NOEXCEPT;
    Link link() const NOEXCEPT;
    operator bool() const NOEXCEPT;

    /// Compares link value only.
    bool operator==(element other) const NOEXCEPT;
    bool operator!=(element other) const NOEXCEPT;

private:
    static constexpr auto link_size = sizeof(Link);
    static constexpr auto key_size = array_count<Key>;
    static constexpr auto value_size = Manager::size;
    static constexpr size_t size(size_t value) NOEXCEPT
    {
        return array_count<Key> + sizeof(Link) + value;
    }

    memory_ptr data() const NOEXCEPT;
    memory_ptr data(size_t byte_offset) const NOEXCEPT;
    void populate(Link next, auto& write, size_t limit) NOEXCEPT;
    void populate(Link next, const Key& key, auto& write,
        size_t limit) NOEXCEPT;

    Link link_;
    Manager& manager_;
};

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Manager, typename Key, typename Link,\
if_key<Key> If1, if_link<Link> If2>
#define CLASS element<Manager, Key, Link, If1, If2>

#include <bitcoin/database/impl/primitives/element.ipp>

#undef CLASS
#undef TEMPLATE

#endif
