/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOMAP_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::nomap(storage& header, storage& body) NOEXCEPT
  : head_(header, 0), body_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    Link count{};
    return head_.create() &&
        head_.get_body_count(count) && body_.truncate(count);
}

TEMPLATE
bool CLASS::close() NOEXCEPT
{
    return head_.set_body_count(body_.count());
}

TEMPLATE
bool CLASS::backup(bool) NOEXCEPT
{
    return head_.set_body_count(body_.count());
}

TEMPLATE
bool CLASS::restore() NOEXCEPT
{
    Link count{};
    return head_.verify() &&
        head_.get_body_count(count) && body_.truncate(count);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return head_.verify() &&
        head_.get_body_count(count) && count == body_.count();
}

// sizing
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::buckets() const NOEXCEPT
{
    return head_.buckets();
}

TEMPLATE
size_t CLASS::head_size() const NOEXCEPT
{
    return head_.size();
}

TEMPLATE
size_t CLASS::body_size() const NOEXCEPT
{
    return body_.size();
}

TEMPLATE
size_t CLASS::capacity() const NOEXCEPT
{
    return body_.capacity();
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return body_.count();
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    return body_.truncate(count);
}

TEMPLATE
bool CLASS::expand(const Link& count) NOEXCEPT
{
    return body_.expand(count);
}

TEMPLATE
bool CLASS::drop() NOEXCEPT
{
    return body_.truncate(0) && backup();
}

TEMPLATE
bool CLASS::reserve(const Link& size) NOEXCEPT
{
    // Not writer-writer thread safe (two writers may share reserve).
    return body_.reserve(size);
}

TEMPLATE
Link CLASS::allocate(const Link& size) NOEXCEPT
{
    return body_.allocate(size);
}

TEMPLATE
memory CLASS::get_memory() const NOEXCEPT
{
    return body_.get();
}

// error condition
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return body_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return body_.get_space();
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return body_.reload();
}

// query interface
// ----------------------------------------------------------------------------

// static
TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const memory& ptr, const Link& link, Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr || link.is_terminal())
        return false;

    const auto start = body::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr.size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    const auto offset = ptr.offset(start);
    if (is_null(offset))
        return false;

    iostream stream{ offset, size - position };
    reader source{ stream };

    if constexpr (!is_slab)
    {
        BC_DEBUG_ONLY(source.set_limit(Size * element.count());)
    }

    return element.from_data(source);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    return get(get_memory(), link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline bool CLASS::put(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Link& link, const Element& element) NOEXCEPT
{
    using namespace system;
    const auto ptr = body_.get(link);
    return put(ptr, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const memory& ptr, const Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr)
        return false;

    iostream stream{ ptr };
    flipper sink{ stream };

    if constexpr (!is_slab)
    {
        BC_DEBUG_ONLY(sink.set_limit(Size * element.count());)
    }

    return element.to_data(sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const memory& ptr, const Link& link,
    const Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr || link.is_terminal())
        return false;

    const auto start = body::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr.size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    const auto offset = ptr.offset(start);
    if (is_null(offset))
        return false;

    iostream stream{ offset, size - position };
    flipper sink{ stream };

    if constexpr (!is_slab)
    {
        BC_DEBUG_ONLY(sink.set_limit(Size * element.count());)
    }

    return element.to_data(sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline bool CLASS::put_link(Link& link, const Element& element) NOEXCEPT
{
    const auto count = element.count();
    link = body_.allocate(count);
    return put(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline Link CLASS::put_link(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, element) ? link : Link{};
}

// NOT THREAD SAFE (used only for height index with writer ordering).
TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline bool CLASS::commit(const Element& element) NOEXCEPT
{
    // Zero allocation provides link of next (presumably reserved) element.
    const auto link = body_.allocate(0);

    // Write element into reserved but unallocated space.
    if (!put(body_.get_capacity(link), element))
        return false;

    // Allocate reserved and written element (exposes logically).
    return !body_.allocate(element.count()).is_terminal();
}

} // namespace database
} // namespace libbitcoin

#endif
