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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOMAP_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::nomap(storage& header, storage& body) NOEXCEPT
  : head_(header, 0), manager_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    Link count{};
    return head_.create() &&
        head_.get_body_count(count) && manager_.truncate(count);
}

TEMPLATE
bool CLASS::close() NOEXCEPT
{
    return head_.set_body_count(manager_.count());
}

TEMPLATE
bool CLASS::backup() NOEXCEPT
{
    return head_.set_body_count(manager_.count());
}

TEMPLATE
bool CLASS::restore() NOEXCEPT
{
    Link count{};
    return head_.verify() &&
        head_.get_body_count(count) && manager_.truncate(count);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return head_.verify() &&
        head_.get_body_count(count) && count == manager_.count();
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
    return manager_.size();
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return manager_.count();
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    return manager_.truncate(count);
}

TEMPLATE
bool CLASS::expand(const Link& count) NOEXCEPT
{
    return manager_.expand(count);
}

// error condition
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return manager_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return manager_.get_space();
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return manager_.reload();
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::reserve(const Link& size) NOEXCEPT
{
    // Reserve not writer-writer thread safe (two writers may share reserve).
    return manager_.reserve(size);
}

TEMPLATE
inline memory_ptr CLASS::get_memory() const NOEXCEPT
{
    return manager_.get();
}

// static
TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const memory_ptr& ptr, const Link& link,
    Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr || link.is_terminal())
        return false;

    const auto start = manager::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr->size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position > size)
        return false;

    const auto offset = ptr->offset(start);
    if (is_null(offset))
        return false;

    iostream stream{ offset, size - position };
    reader source{ stream };

    if constexpr (!is_slab) { BC_DEBUG_ONLY(source.set_limit(Size * element.count());) }

    ///////////////////////////////////////////////////////////////////////////
    // TODO: shared lock.
    ///////////////////////////////////////////////////////////////////////////
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
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream stream{ *ptr };
    flipper sink{ stream };

    if constexpr (!is_slab) { BC_DEBUG_ONLY(sink.set_limit(Size * element.count());) }

    ///////////////////////////////////////////////////////////////////////////
    // TODO: unique lock.
    ///////////////////////////////////////////////////////////////////////////
    return element.to_data(sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline bool CLASS::put_link(Link& link, const Element& element) NOEXCEPT
{
    ///////////////////////////////////////////////////////////////////////////
    // BUGBUG: when the table is directly accessible allocate/put is not
    // BUGBUG: reader-writer thread safe (non-atomic), ok when not linked.
    // BUGBUG: so use template bool to indicate guard using reader-writer lock.
    ///////////////////////////////////////////////////////////////////////////
    const auto count = element.count();
    link = manager_.allocate(count);
    return put(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
inline Link CLASS::put_link(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, element) ? link : Link{};
}

} // namespace database
} // namespace libbitcoin

#endif
