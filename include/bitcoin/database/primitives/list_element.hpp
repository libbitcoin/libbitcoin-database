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
#ifndef LIBBITCOIN_DATABASE_LIST_ELEMENT_HPP
#define LIBBITCOIN_DATABASE_LIST_ELEMENT_HPP

#include <cstddef>
#include <cstdint>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// A hash table key-conflict row, implemented as a linked list.
/// Link cannot exceed 64 bits. A default Key creates an unkeyed list.
template <typename Manager, typename Link, typename Key>
class list_element
{
public:
    typedef byte_serializer::functor write_function;
    typedef byte_deserializer::functor read_function;
    static const auto not_found = (Link)bc::max_uint64;

    /// The stored size of a value with the given size.
    static size_t size(size_t value_size);

    /// Construct for a new element.
    list_element(Manager& manager, system::shared_mutex& mutex);

    /// Construct for an existing element.
    list_element(Manager& manager, Link link, system::shared_mutex& mutex);

    /// Allocate and populate a new unkeyed record element.
    Link create(write_function write);

    /// Allocate and populate a new keyed record element.
    Link create(const Key& key, write_function write);

    /// Allocate and populate a new keyed slab element.
    Link create(const Key& key, write_function write, size_t value_size);

    /// Update this element to the next element (read next from file).
    bool jump_next();

    /// Convert the instance into a terminator.
    void terminate();

    /// Connect the next element (write to file).
    void set_next(Link next) const;

    /// Write to the state of the element (write to file).
    void write(write_function writer) const;

    /// Read from the state of the element.
    void read(read_function reader) const;

    /// True if the element key (read from file) matches the parameter.
    bool match(const Key& key) const;

    /// The key of this element (read from file).
    Key key() const;

    /// The address of this element.
    Link link() const;

    /// The address of the next element (read from file).
    Link next() const;

    /// A list terminator for this instance.
    list_element terminator() const;

    /// The element is terminal (not found, cannot be read).
    bool terminal() const;

    /// Cast operator, true if element was found (not terminal).
    operator bool() const;

    /// Equality comparison operators, compares link value only.
    bool operator==(list_element other) const;
    bool operator!=(list_element other) const;

private:
    memory_ptr data(size_t bytes) const;
    void initialize(const Key& key, write_function write);

    Link link_;
    Manager& manager_;
    system::shared_mutex& mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/list_element.ipp>

#endif
