/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <cstring>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
INLINE CLASS::iterator(const manager& memory, const Link& start,
    const Key& key) NOEXCEPT
  : manager_(memory), key_(key), link_(start)
{
    const auto ptr = get_ptr();
    if (!is_match(ptr))
        advance(ptr);
}

TEMPLATE
INLINE bool CLASS::advance() NOEXCEPT
{
    return advance(get_ptr());
}

TEMPLATE
INLINE const Link& CLASS::self() const NOEXCEPT
{
    return link_;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
INLINE memory_ptr CLASS::get_ptr() const NOEXCEPT
{
    return manager_.get();
}

TEMPLATE
INLINE bool CLASS::advance(const memory_ptr& ptr) NOEXCEPT
{
    if (link_.is_terminal() || !ptr)
        return false;

    do
    {
        link_ = get_next(ptr);
        if (is_match(ptr))
            return true;
    }
    while (!link_.is_terminal());
    return false;
}

TEMPLATE
INLINE bool CLASS::is_match(const memory_ptr& ptr) const NOEXCEPT
{
    if (link_.is_terminal() || !ptr)
        return false;

    BC_ASSERT(!system::is_add_overflow(link_to_position(link_), Link::size));
    const auto position = ptr->offset(link_to_position(link_) + Link::size);
    if (is_null(position))
        return false;

    return is_zero(std::memcmp(key_.data(), position, key_.size()));
}

TEMPLATE
INLINE Link CLASS::get_next(const memory_ptr& ptr) const NOEXCEPT
{
    if (link_.is_terminal() || !ptr)
        return Link::terminal;

    const auto position = ptr->offset(link_to_position(link_));
    if (is_null(position))
        return Link::terminal;

    return { system::unsafe_array_cast<uint8_t, Link::size>(position) };
}

// private
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::link_to_position(const Link& link) NOEXCEPT
{
    using namespace system;
    const auto value = possible_narrow_cast<size_t>(link.value);

    if constexpr (is_slab)
    {
        // Slab implies link/key incorporated into size.
        return value;
    }
    else
    {
        // Record implies link/key independent of Size.
        constexpr auto element_size = Link::size + array_count<Key> + Size;
        BC_ASSERT(!is_multiply_overflow(value, element_size));
        return value * element_size;
    }
}

} // namespace database
} // namespace libbitcoin

#endif
