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
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

// unkeyed: [[]   [link][value]]
// keyed:   [[key][link][value]]
// Value may be fixed (slab element) or variable (record element).
// Manager provides random access read and write append to memory map.

namespace libbitcoin {
namespace database {
namespace primitives {

TEMPLATE
CLASS::element(Manager& manager) NOEXCEPT
  : manager_(manager), link_(eof)
{
}

TEMPLATE
CLASS::element(Manager& manager, Link link) NOEXCEPT
  : manager_(manager), link_(link)
{
}

// element append
// ----------------------------------------------------------------------------
// TODO: create 4 derived mixin classes, adding each of these.

TEMPLATE
Link CLASS::append_unkeyed_record(Link next, auto& write) NOEXCEPT
{
    static_assert(is_zero(key_size));
    link_ = manager_.allocate(one);
    populate(next, write, value_size);
    return link_;
}

TEMPLATE
Link CLASS::append_keyed_record(Link next, const Key& key, auto& write) NOEXCEPT
{
    static_assert(!is_zero(key_size));
    link_ = manager_.allocate(one);
    populate(next, key, write, value_size);
    return link_;
}

TEMPLATE
Link CLASS::append_unkeyed_slab(Link next, auto& write, size_t limit) NOEXCEPT
{
    static_assert(is_zero(key_size));
    link_ = manager_.allocate(size(limit));
    populate(next, write, limit);
    return link_;
}

TEMPLATE
Link CLASS::append_keyed_slab(Link next, const Key& key, auto& write,
    size_t limit) NOEXCEPT
{
    static_assert(!is_zero(key_size));
    link_ = manager_.allocate(size(limit));
    populate(next, key, write, limit);
    return link_;
}

TEMPLATE
void CLASS::read_record(auto& read) const NOEXCEPT
{
    read_slab(read, value_size);
}

TEMPLATE
void CLASS::read_slab(auto& read, size_t limit) const NOEXCEPT
{
    // Advance to payload address.
    const auto memory = data(key_size + link_size);
    const auto start = memory->buffer();
    system::read::bytes::copy reader({ start, std::next(start, limit) });
    read(reader);
}

// navigation
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::advance() NOEXCEPT
{
    if (link_ == eof)
        return false;

    // Advance to link address.
    const auto memory = data(key_size);
    link_ = system::unsafe_from_little_endian<Link>(memory->buffer());
    return true;
}

TEMPLATE
bool CLASS::match(const Key& key) const NOEXCEPT
{
    const auto memory = data();
    return std::equal(key.begin(), key.end(), memory->buffer());
}

// properties
// ----------------------------------------------------------------------------

TEMPLATE
Key CLASS::key() const NOEXCEPT
{
    const auto memory = data();
    return system::unsafe_array_cast<uint8_t, key_size>(memory->buffer());
}

TEMPLATE
Link CLASS::link() const NOEXCEPT
{
    return link_;
}

// operators
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::operator bool() const NOEXCEPT
{
    return link_ != eof;
}

TEMPLATE
bool CLASS::operator==(element other) const NOEXCEPT
{
    return link_ == other.link_;
}

TEMPLATE
bool CLASS::operator!=(element other) const NOEXCEPT
{
    return link_ != other.link_;
}

// private
// ----------------------------------------------------------------------------

TEMPLATE
memory_ptr CLASS::data() const NOEXCEPT
{
    BC_ASSERT(link_ != eof);
    return manager_.get(link_);
}

TEMPLATE
memory_ptr CLASS::data(size_t byte_offset) const NOEXCEPT
{
    auto memory = data();
    memory->increment(byte_offset);
    return memory;
}

TEMPLATE
void CLASS::populate(Link next, auto& write, size_t limit) NOEXCEPT
{
    const auto memory = data();
    auto start = memory->buffer();
    system::write::bytes::copy writer({ start, std::next(start, limit) });
    writer.write_little_endian<Link>(next);
    write(writer);
}

TEMPLATE
void CLASS::populate(Link next, const Key& key, auto& write,
    size_t limit) NOEXCEPT
{
    const auto memory = data();
    auto start = memory->buffer();
    system::write::bytes::copy writer({ start, std::next(start, limit) });
    writer.write_bytes(key);
    writer.write_little_endian<Link>(next);
    write(writer);
}

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#endif
