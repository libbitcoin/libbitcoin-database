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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP

#include <iterator>
#include <cstring>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::iterator(memory_ptr&& data, const Link& start, Key&& key) NOEXCEPT
  : memory_(std::move(data)), key_(std::forward<Key>(key)),
    link_(to_first(start))
{
}

TEMPLATE
CLASS::iterator(memory_ptr&& data, const Link& start, const Key& key) NOEXCEPT
  : memory_(std::move(data)), key_(key), link_(to_first(start))
{
}

TEMPLATE
inline bool CLASS::advance() NOEXCEPT
{
    return !((link_ = to_next(link_))).is_terminal();
}

TEMPLATE
inline const Key& CLASS::key() const NOEXCEPT
{
    return key_;
}

TEMPLATE
inline const Link& CLASS::get() const NOEXCEPT
{
    return link_;
}

TEMPLATE
inline const memory_ptr& CLASS::ptr() const NOEXCEPT
{
    return memory_;
}

TEMPLATE
inline void CLASS::reset() NOEXCEPT
{
    link_ = Link::terminal;
    memory_.reset();
}

// operators
// ----------------------------------------------------------------------------

TEMPLATE
inline CLASS::operator bool() const NOEXCEPT
{
    return !link_.is_terminal();
}

TEMPLATE
inline const Link& CLASS::operator*() const NOEXCEPT
{
    return link_;
}

TEMPLATE
inline const Link* CLASS::operator->() const NOEXCEPT
{
    return &link_;
}

TEMPLATE
inline CLASS& CLASS::operator++() NOEXCEPT
{
    advance();
    return *this;
}

TEMPLATE
inline CLASS CLASS::operator++(int) NOEXCEPT
{
    auto previous = *this;
    ++(*this);
    return previous;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::to_first(Link link) const NOEXCEPT
{
    // Because of this !link_.is_terminal() subsequently guards both.
    if (!memory_)
        return Link::terminal;

    while (!link.is_terminal())
    {
        // get element offset (fault)
        const auto offset = memory_->offset(manager::link_to_position(link));
        if (is_null(offset))
            return Link::terminal;

        // element key matches (found)
        if (keys::compare(system::unsafe_array_cast<uint8_t, key_size>(
            std::next(offset, Link::size)), key_))
            return link;

        // set next element link (loop)
        link = system::unsafe_array_cast<uint8_t, Link::size>(offset);
    }

    return link;
}

TEMPLATE
Link CLASS::to_next(Link link) const NOEXCEPT
{
    while (!link.is_terminal())
    {
        // get element offset (fault)
        auto offset = memory_->offset(manager::link_to_position(link));
        if (is_null(offset))
            return Link::terminal;

        // set next element link (loop)
        link = { system::unsafe_array_cast<uint8_t, Link::size>(offset) };
        if (link.is_terminal())
            return link;

        // get next element offset (fault)
        offset = memory_->offset(manager::link_to_position(link));
        if (is_null(offset))
            return Link::terminal;

        // next element key matches (found)
        if (keys::compare(system::unsafe_array_cast<uint8_t, key_size>(
            std::next(offset, Link::size)), key_))
            return link;
    }

    return link;
}

} // namespace database
} // namespace libbitcoin

#endif
