/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::arraymap(storage& header, storage& body) NOEXCEPT
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

// query interface (iostreams)
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    auto source = getter(link);
    return source && element.from_data(*source);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put_link(Link& link, const Element& element) NOEXCEPT
{
    auto sink = creater(link, element.count());
    return sink && element.to_data(*sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::put_link(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, element) ? link : Link{};
}

// query interface (memory)
// ============================================================================

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get1(const Link& link, Element& element) const NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr) return false;
    simple_reader reader{ *ptr };
    return element.from_data(reader);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put1(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link1(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put_link1(Link& link, const Element& element) NOEXCEPT
{
    link = manager_.allocate(element.count());
    const auto ptr = manager_.get(link);
    if (!ptr) return false;
    simple_writer writer{ *ptr };
    return element.to_data(writer);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::put_link1(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link1(link, element) ? link : Link{};
}

// ============================================================================

// protected
// ----------------------------------------------------------------------------

TEMPLATE
reader_ptr CLASS::getter(const Link& link) const NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr)
        return {};

    const auto source = std::make_shared<reader>(ptr);

    // Limits to single record or eof for slab (caller can remove limit).
    if constexpr (!is_slab) { source->set_limit(Size); }
    return source;
}

TEMPLATE
writer_ptr CLASS::creater(Link& link, const Link& size) NOEXCEPT
{
    link = manager_.allocate(size);
    const auto ptr = manager_.get(link);
    if (!ptr)
        return {};

    const auto sink = std::make_shared<writer>(ptr);

    // Limits to size records or eof for slab.
    if constexpr (!is_slab) { sink->set_limit(Size * size); }
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
