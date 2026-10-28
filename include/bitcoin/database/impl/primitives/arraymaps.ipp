/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAPS_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAPS_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::arraymaps(storage& header, storage& body,
    const Link& buckets) NOEXCEPT
  : head_(header, buckets),
    body_(body)
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
bool CLASS::enabled() const NOEXCEPT
{
    return head_.enabled();
}

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
Link CLASS::count() const NOEXCEPT
{
    return body_.count();
}

TEMPLATE
Link CLASS::allocate(const Link& count) NOEXCEPT
{
    return body_.allocate(count);
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    return body_.truncate(count);
}

TEMPLATE
bool CLASS::drop() NOEXCEPT
{
    return head_.clear() && body_.truncate(0) && backup();
}

// Faults.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    const auto ec = head_.get_fault();
    return ec ? ec : body_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return system::ceilinged_add(head_.get_space(), body_.get_space());
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return body_.reload();
}

// query interface (head)
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::at(size_t key) const NOEXCEPT
{
    return head_.at(key);
}

TEMPLATE
bool CLASS::push(const Link& link, size_t key) NOEXCEPT
{
    // Avoid setting at/above terminal sentinel into a bucket position.
    if (key >= Link::terminal)
        return false;

    return head_.push(link, head_.index(key));
}

// query interface (columnar)
// ----------------------------------------------------------------------------

TEMPLATE
memory CLASS::guard() const NOEXCEPT
{
    return get_memory<zero>();
}

TEMPLATE
template <size_t Column>
memory CLASS::get_memory() const NOEXCEPT
{
    return body_.template get<Column>();
}

TEMPLATE
template <size_t Column>
memory::iterator CLASS::get_raw() const NOEXCEPT
{
    if (is_zero(count().value))
        return {};

    return body_.template get_raw<Column>(Link{ 0 });
}

// static
TEMPLATE
template <size_t Column, typename Element>
bool CLASS::get(const memory& ptr, const Link& link, Element& element) NOEXCEPT
{
    static_assert(Element::size == width<Column>);
    if (!ptr || link.is_terminal())
        return false;

    using namespace system;
    const auto start = body::template link_to_position<Column>(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr.size();
    const auto position = possible_narrow_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    const auto offset = ptr.offset(start);
    if (is_null(offset))
        return false;

    iostream stream{ offset, size - position };
    reader source{ stream };

    BC_DEBUG_ONLY(source.set_limit(width<Column> * element.count());)
    return element.from_data(source);
}

// TODO: gets are not optimized for shared remap guard.
TEMPLATE
template <size_t Column, typename Element>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    return get<Column>(get_memory<Column>(), link, element);
}

// TODO: puts are optimized for shared remap guard (required).
TEMPLATE
template <size_t Column, typename Element>
bool CLASS::put(const Link& link, const Element& element) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // Completes the allocation, written or abandoned.
    const auto it = body_.template get_raw<Column>(link);
    const auto written = put<Column>(it, element);
    body_.complete(link, element.count());
    return written;
}

TEMPLATE
void CLASS::complete(const Link& link, const Link& count) NOEXCEPT
{
    body_.complete(link, count);
}

// protected (unguarded memory access)
TEMPLATE
template <size_t Column, typename Element>
bool CLASS::put(memory::iterator it, const Element& element) NOEXCEPT
{
    static_assert(Element::size == width<Column>);
    if (is_null(it))
        return false;

    using namespace system;
    const auto bytes = width<Column> * element.count();
    iostream stream{ it, possible_narrow_sign_cast<ptrdiff_t>(bytes) };
    flipper sink{ stream };

    BC_DEBUG_ONLY(sink.set_limit(width<Column> * element.count());)
    return element.to_data(sink);
}

} // namespace database
} // namespace libbitcoin

#endif
