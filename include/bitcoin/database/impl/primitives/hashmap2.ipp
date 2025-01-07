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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP2_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP2_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashmap2(storage& header, storage& body, const Link& buckets) NOEXCEPT
  : head_(header, buckets), manager_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    Link count{};
    return head_.create() && head_.get_body_count(count) &&
        manager_.truncate(count);
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
    return head_.verify() && head_.get_body_count(count) &&
        manager_.truncate(count);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return head_.verify() && head_.get_body_count(count) &&
        (count == manager_.count());
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
    return manager_.size();
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return manager_.count();
}

// query interface
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
Link CLASS::top(const Link& link) const NOEXCEPT
{
    if (link >= head_.buckets())
        return {};

    return head_.top(link);
}

TEMPLATE
bool CLASS::exists(const Key& key) const NOEXCEPT
{
    return !first(key).is_terminal();
}

TEMPLATE
Link CLASS::first(const Key& key) const NOEXCEPT
{
    return head_.top(key);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::find(const Key& key, Element& element) const NOEXCEPT
{
    // This override avoids duplicated memory_ptr construct in get(first()).
    const auto ptr = manager_.get();
    return read(ptr, first(ptr, head_.top(key), key), element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    // This override is the normal form.
    return read(manager_.get(), link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Key& key, const Element& element) NOEXCEPT
{
    using namespace system;
    const auto count = element.count();
    const auto link = allocate(count);
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    // iostream.flush is a nop (direct copy).
    iostream stream{ *ptr };
    finalizer sink{ stream };
    sink.skip_bytes(Link::size);
    sink.write_bytes(key);

    if constexpr (!is_slab) { BC_DEBUG_ONLY(sink.set_limit(Size * count);) }
    return element.to_data(sink) && head_.push(link, head_.index(key));
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::read(const memory_ptr& ptr, const Link& link,
    Element& element) NOEXCEPT
{
    if (!ptr || link.is_terminal())
        return false;

    using namespace system;
    const auto start = manager::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr->size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position > size)
        return false;

    const auto offset = ptr->offset(position);
    if (is_null(offset))
        return false;

    // Stream starts at record and the index is skipped for reader convenience.
    iostream stream{ offset, size - position };
    reader source{ stream };

    if constexpr (!is_slab) { BC_DEBUG_ONLY(source.set_limit(Size);) }
    return element.from_data(source);
}

} // namespace database
} // namespace libbitcoin

#endif
