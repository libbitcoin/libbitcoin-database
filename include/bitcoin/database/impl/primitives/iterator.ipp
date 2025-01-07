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
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::iterator(const memory_ptr& data, const Link& start,
    const Key& key) NOEXCEPT
  : memory_(data), key_(key), link_(to_match(start))
{
}

TEMPLATE
bool CLASS::advance() NOEXCEPT
{
    return !((link_ = to_next(link_))).is_terminal();
}

TEMPLATE
const Link& CLASS::self() const NOEXCEPT
{
    return link_;
}

TEMPLATE
const memory_ptr& CLASS::get() const NOEXCEPT
{
    return memory_;
}

TEMPLATE
CLASS::operator bool() const NOEXCEPT
{
    return !link_.is_terminal();
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::to_match(Link link) const NOEXCEPT
{
    // Because of this !link_.is_terminal() subsequently guards both.
    if (!memory_)
        return {};

    while (!link.is_terminal())
    {
        // get element offset (fault)
        const auto offset = memory_->offset(manager::link_to_position(link));
        if (is_null(offset))
            return {};

        // element key matches (found)
        const auto key_ptr = std::next(offset, Link::size);
        if (is_zero(std::memcmp(key_.data(), key_ptr, array_count<Key>)))
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
            return {};

        // set next element link (loop)
        link = { system::unsafe_array_cast<uint8_t, Link::size>(offset) };
        if (link.is_terminal())
            return link;

        // get next element offset (fault)
        offset = memory_->offset(manager::link_to_position(link));
        if (is_null(offset))
            return {};

        // next element key matches (found)
        const auto key_ptr = std::next(offset, Link::size);
        if (is_zero(std::memcmp(key_.data(), key_ptr, array_count<Key>)))
            return link;
    }

    return link;
}

} // namespace database
} // namespace libbitcoin

#endif
