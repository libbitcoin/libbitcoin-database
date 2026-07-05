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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOMAPS_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOMAPS_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::nomaps(storage& header, storage& body) NOEXCEPT
  : head_(header, 0),
    manager_(body)
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
bool CLASS::backup(bool) NOEXCEPT
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
Link CLASS::allocate(const Link& count) NOEXCEPT
{
    return manager_.allocate(count);
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    return manager_.truncate(count);
}

// Faults.
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
template <size_t Column>
memory_ptr CLASS::get_memory() const NOEXCEPT
{
    return manager_.template get<Column>();
}

// static
TEMPLATE
template <size_t Column, typename Element>
bool CLASS::get(const memory_ptr& ptr, const Link& link,
    Element& element) NOEXCEPT
{
    static_assert(Element::size == width<Column>, "element size != width");
    using namespace system;
    if (!ptr || link.is_terminal())
        return false;

    const auto start = body::template link_to_position<Column>(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr->size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    const auto offset = ptr->offset(start);
    if (is_null(offset))
        return false;

    iostream stream{ offset, size - position };
    reader source{ stream };

    BC_DEBUG_ONLY(source.set_limit(width<Column> * element.count());)
    return element.from_data(source);
}

TEMPLATE
template <size_t Column, typename Element>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    return get<Column>(get_memory<Column>(), link, element);
}

TEMPLATE
template <size_t Column, typename Element>
bool CLASS::put(const Link& link, const Element& element) NOEXCEPT
{
    using namespace system;
    const auto ptr = manager_.template get<Column>(link);
    return put<Column>(ptr, element);
}

TEMPLATE
template <size_t Column, typename Element>
bool CLASS::put(const memory_ptr& ptr, const Element& element) NOEXCEPT
{
    static_assert(Element::size == width<Column>, "element size != width");
    using namespace system;
    if (!ptr)
        return false;

    iostream stream{ *ptr };
    flipper sink{ stream };

    BC_DEBUG_ONLY(sink.set_limit(width<Column> * element.count());)
    return element.to_data(sink);
}

} // namespace database
} // namespace libbitcoin

#endif
