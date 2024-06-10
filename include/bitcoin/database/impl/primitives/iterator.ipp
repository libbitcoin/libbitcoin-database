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

#include <algorithm>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
INLINE CLASS::iterator(const memory_ptr& data, const Link& start,
    const Key& key) NOEXCEPT
  : memory_(data), key_(key), link_(to_match(start))
{
}

TEMPLATE
INLINE bool CLASS::advance() NOEXCEPT
{
    return !((link_ = to_next(link_))).is_terminal();
}

TEMPLATE
INLINE const Link& CLASS::self() const NOEXCEPT
{
    return link_;
}

TEMPLATE
INLINE const memory_ptr& CLASS::get() const NOEXCEPT
{
    return memory_;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
INLINE Link CLASS::to_match(Link link) const NOEXCEPT
{
    // Because of this !link_.is_terminal() subsequently guards both.
    if (!memory_)
        return {};

    while (!link.is_terminal())
    {
        // get element offset (fault)
        auto offset = memory_->offset(link_to_position(link));
        if (is_null(offset))
            return {};

        // element key matches (found)
        const auto key = std::next(offset, Link::size);
        if (is_zero(std::memcmp(key_.data(), key, key_size)))
            return std::move(link);

        // set next element link (loop)
        link = system::unsafe_array_cast<uint8_t, Link::size>(offset);
    }

    return std::move(link);
}

TEMPLATE
INLINE Link CLASS::to_next(Link link) const NOEXCEPT
{
    while (!link.is_terminal())
    {
        // get element offset (fault)
        auto offset = memory_->offset(link_to_position(link));
        if (is_null(offset))
            return {};

        // set next element link (loop)
        link = { system::unsafe_array_cast<uint8_t, Link::size>(offset) };
        if (link.is_terminal())
            return std::move(link);

        // get next element offset (fault)
        offset = memory_->offset(link_to_position(link));
        if (is_null(offset))
            return {};

        // next element key matches (found)
        const auto key = std::next(offset, Link::size);
        if (is_zero(std::memcmp(key_.data(), key, key_size)))
            return std::move(link);
    }

    return std::move(link);
}

////TEMPLATE
////INLINE bool CLASS::is_match() const NOEXCEPT
////{
////    if (link_.is_terminal())
////        return false;
////
////    const auto offset = memory_->offset(link_to_position(link_));
////    if (is_null(offset))
////        return false;
////
////    const auto key = std::next(offset, Link::size);
////    return is_zero(std::memcmp(key_.data(), key, key_size));
////}
////
////TEMPLATE
////INLINE Link CLASS::get_next() const NOEXCEPT
////{
////    if (link_.is_terminal())
////        return {};
////
////    const auto offset = memory_->offset(link_to_position(link_));
////    if (is_null(offset))
////        return {};
////
////    return { system::unsafe_array_cast<uint8_t, Link::size>(offset) };
////}

// private
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::link_to_position(const Link& link) NOEXCEPT
{
    const auto value = system::possible_narrow_cast<size_t>(link.value);

    if constexpr (is_slab)
    {
        // Slab implies link/key incorporated into size.
        return value;
    }
    else
    {
        // Record implies link/key independent of Size.
        constexpr auto element_size = Link::size + array_count<Key> + Size;
        BC_ASSERT(!system::is_multiply_overflow(value, element_size));

        return value * element_size;
    }
}

} // namespace database
} // namespace libbitcoin

#endif
