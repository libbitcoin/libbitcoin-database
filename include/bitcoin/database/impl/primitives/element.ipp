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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP

#include <algorithm>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

// Element is a block of store memory with the following layout.
// An empty-valued key is used for simple (non-hash-table) linked lists.
// Elements form a forward-navigable linked list with 'not_found' terminator.
// Elements of a common mutex support read/write concurrency, though updateable
// portions of payload must be protected by caller within each reader/writer.
// [ Key  ]
// [ Link ]
// [ payload... ]

// Parameterizing Manager allows const and non-const.
TEMPLATE
CLASS::list_element(Manager& manager, shared_mutex& mutex) NOEXCEPT
  : manager_(manager), link_(not_found), mutex_(mutex)
{
}

TEMPLATE
CLASS::list_element(Manager& manager, Link link, shared_mutex& mutex) NOEXCEPT
  : manager_(manager), link_(link), mutex_(mutex)
{
}

// TODO: pass extent of the write.
// private
// Populate a new (unlinked) element with key and value data.
TEMPLATE
void CLASS::initialize(const Key& key, auto& write) NOEXCEPT
{
    using namespace system;
    const auto memory = data(zero);
    auto start = memory->buffer();

    // Limited to tuple|iterator Key types.
    unsafe_array_cast<uint8_t, key_size>(start) = key;
    std::advance(start, key_size + link_size);

    const auto size = 42u;
    const auto end = std::next(start, size);
    write::bytes::copy writer({ start, end });
    write(writer);
}

// This call assumes the manager is a record_manager.
TEMPLATE
Link CLASS::create(auto& write) NOEXCEPT
{
    constexpr empty_key unkeyed{};
    link_ = manager_.allocate(one);
    initialize(unkeyed, write);
    return link_;
}

// This call assumes the manager is a record_manager.
TEMPLATE
Link CLASS::create(const Key& key, auto& write) NOEXCEPT
{
    link_ = manager_.allocate(one);
    initialize(key, write);
    return link_;
}

// This call assumes the manager is a slab_manager.
TEMPLATE
Link CLASS::create(const Key& key, auto& write, size_t value_size) NOEXCEPT
{
    link_ = manager_.allocate(size(value_size));
    initialize(key, write);
    return link_;
}

// TODO: pass extent of the write.
TEMPLATE
void CLASS::write(auto& write, size_t limit) const NOEXCEPT
{
    using namespace system;
    const auto memory = data(key_size + link_size);
    const auto start = memory->buffer();
    const auto end = std::next(start, limit);
    write::bytes::copy writer({ start, end });
    write(writer);
}

// Jump to the next element in the list.
TEMPLATE
bool CLASS::jump_next() NOEXCEPT
{
    if (link_ == not_found)
        return false;

    link_ = next();
    return true;
}

// Convert the instance into a terminator.
TEMPLATE
void CLASS::terminate() NOEXCEPT
{
    link_ = not_found;
}

// Populate next link value.
TEMPLATE
void CLASS::set_next(Link next) const NOEXCEPT
{
    const auto memory = data(key_size);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);
    system::unsafe_to_little_endian<Link>(memory->buffer(), next);
    ///////////////////////////////////////////////////////////////////////////
}

// TODO: pass extent of the read.
TEMPLATE
void CLASS::read(auto& read, size_t limit) const NOEXCEPT
{
    using namespace system;
    const auto memory = data(key_size + link_size);
    const auto start = memory->buffer();
    const auto end = std::next(start, limit);
    read::bytes::copy reader({ start, end });
    read(reader);
}

TEMPLATE
bool CLASS::match(const Key& key) const NOEXCEPT
{
    const auto memory = data(zero);
    return std::equal(key.begin(), key.end(), memory->buffer());
}

TEMPLATE
Key CLASS::key() const NOEXCEPT
{
    // Limited to tuple Key types (see deserializer to generalize).
    const auto memory = data(zero);
    return system::unsafe_array_cast<uint8_t, key_size>(memory->buffer());
}

TEMPLATE
Link CLASS::link() const NOEXCEPT
{
    return link_;
}

TEMPLATE
Link CLASS::next() const NOEXCEPT
{
    const auto memory = data(key_size);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return system::unsafe_from_little_endian<Link>(memory->buffer());
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
CLASS CLASS::terminator() const NOEXCEPT
{
    return { manager_, mutex_ };
}

TEMPLATE
bool CLASS::terminal() const NOEXCEPT
{
    return link_ == not_found;
}

// operators
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::operator bool() const NOEXCEPT
{
    return !terminal();
}

TEMPLATE
bool CLASS::operator==(list_element other) const NOEXCEPT
{
    return link_ == other.link_;
}

TEMPLATE
bool CLASS::operator!=(list_element other) const NOEXCEPT
{
    return !(link_ == other.link_);
}

// private
// ----------------------------------------------------------------------------

TEMPLATE
memory_ptr CLASS::data(size_t bytes) const NOEXCEPT
{
    BC_ASSERT(link_ != not_found);
    auto memory = manager_.get(link_);
    memory->increment(bytes);
    return memory;
}

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#endif
