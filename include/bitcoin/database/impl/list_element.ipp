/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_LIST_ELEMENT_IPP
#define LIBBITCOIN_DATABASE_LIST_ELEMENT_IPP

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

// Element is a block of store memory with the following layout.
// An empty-valued key is used for simple (non-hash-table) linked lists.
// Elements form a forward-navigable linked list with 'not_found' terminator.
// Elements of a common mutex support read/write concurrency, though updateable
// portions of payload must be protected by caller within each reader/writer.
// [ Key  ]
// [ Link ]
// [ payload... ]

// static
template <typename Manager, typename Link, typename Key>
size_t list_element<Manager, Link, Key>::size(size_t value_size)
{
    return std::tuple_size<Key>::value + sizeof(Link) + value_size;
}

// Parameterizing Manager allows const and non-const.
template <typename Manager, typename Link, typename Key>
list_element<Manager, Link, Key>::list_element(Manager& manager,
    shared_mutex& mutex)
  : manager_(manager), link_(not_found), mutex_(mutex)
{
}

template <typename Manager, typename Link, typename Key>
list_element<Manager, Link, Key>::list_element(Manager& manager, Link link,
    shared_mutex& mutex)
  : manager_(manager), link_(link), mutex_(mutex)
{
}

// private
// Populate a new (unlinked) element with key and value data.
template <typename Manager, typename Link, typename Key>
void list_element<Manager, Link, Key>::initialize(const Key& key,
    write_function write)
{
    const auto memory = data(0);
    auto serial = make_unsafe_serializer(memory->buffer());

    // Limited to tuple|iterator Key types.
    serial.write_forward(key);
    serial.skip(sizeof(Link));
    serial.write_delegated(write);
}

// This call assumes the manager is a record_manager.
template <typename Manager, typename Link, typename Key>
Link list_element<Manager, Link, Key>::create(write_function write)
{
    BC_CONSTEXPR empty_key unkeyed{};
    link_ = manager_.allocate(1);
    initialize(unkeyed, write);
    return link_;
}

// This call assumes the manager is a record_manager.
template <typename Manager, typename Link, typename Key>
Link list_element<Manager, Link, Key>::create(const Key& key,
    write_function write)
{
    link_ = manager_.allocate(1);
    initialize(key, write);
    return link_;
}

// This call assumes the manager is a slab_manager.
template <typename Manager, typename Link, typename Key>
Link list_element<Manager, Link, Key>::create(const Key& key,
    write_function write, size_t value_size)
{
    link_ = manager_.allocate(size(value_size));
    initialize(key, write);
    return link_;
}

template <typename Manager, typename Link, typename Key>
void list_element<Manager, Link, Key>::write(write_function writer)
{
    const auto memory = data(std::tuple_size<Key>::value + sizeof(Link));
    auto serial = make_unsafe_serializer(memory->buffer());
    writer(serial);
}

// Jump to the next element in the list.
template <typename Manager, typename Link, typename Key>
bool list_element<Manager, Link, Key>::jump_next()
{
    if (link_ == not_found)
        return false;

    link_ = next();
    return true;
}

// Convert the instance into a terminator.
template <typename Manager, typename Link, typename Key>
void list_element<Manager, Link, Key>::terminate()
{
    link_ = not_found;
}

// Populate next link value.
template <typename Manager, typename Link, typename Key>
void list_element<Manager, Link, Key>::set_next(Link next)
{
    const auto memory = data(std::tuple_size<Key>::value);
    auto serial = make_unsafe_serializer(memory->buffer());

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);
    serial.template write_little_endian<Link>(next);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Manager, typename Link, typename Key>
void list_element<Manager, Link, Key>::read(read_function reader) const
{
    const auto memory = data(std::tuple_size<Key>::value + sizeof(Link));
    auto deserial = make_unsafe_deserializer(memory->buffer());
    reader(deserial);
}

template <typename Manager, typename Link, typename Key>
bool list_element<Manager, Link, Key>::match(const Key& key) const
{
    const auto memory = data(0);
    return std::equal(key.begin(), key.end(), memory->buffer());
}

template <typename Manager, typename Link, typename Key>
Key list_element<Manager, Link, Key>::key() const
{
    const auto memory = data(0);
    auto deserial = make_unsafe_deserializer(memory->buffer());

    // Limited to tuple Key types (see deserializer to generalize).
    return deserial.template read_forward<Key>();
}

template <typename Manager, typename Link, typename Key>
Link list_element<Manager, Link, Key>::link() const
{
    return link_;
}

template <typename Manager, typename Link, typename Key>
Link list_element<Manager, Link, Key>::next() const
{
    const auto memory = data(std::tuple_size<Key>::value);
    auto deserial = make_unsafe_deserializer(memory->buffer());

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);
    return deserial.template read_little_endian<Link>();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Manager, typename Link, typename Key>
list_element<Manager, Link, Key>
list_element<Manager, Link, Key>::terminator() const
{
    return { manager_, mutex_ };
}

template <typename Manager, typename Link, typename Key>
bool list_element<Manager, Link, Key>::terminal() const
{
    return link_ == not_found;
}

template <typename Manager, typename Link, typename Key>
list_element<Manager, Link, Key>::operator bool() const
{
    return !terminal();
}

template <typename Manager, typename Link, typename Key>
bool list_element<Manager, Link, Key>::operator==(list_element other) const
{
    return link_ == other.link_;
}

template <typename Manager, typename Link, typename Key>
bool list_element<Manager, Link, Key>::operator!=(list_element other) const
{
    return !(link_ == other.link_);
}

// private
template <typename Manager, typename Link, typename Key>
memory_ptr list_element<Manager, Link, Key>::data(size_t bytes) const
{
    BITCOIN_ASSERT(link_ != not_found);
    auto memory = manager_.get(link_);
    memory->increment(bytes);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
