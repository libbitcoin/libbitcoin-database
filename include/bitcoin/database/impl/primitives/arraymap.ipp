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
  : header_(header, zero), body_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

// TODO: add header_ (size only), create, verify, snap.
// TODO: invoke header_.set_body_count(count) on close.

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    // TODO: verify empty and set link size into header, snap body (in case files exist).
    ////return header_.create() && verify();
    return true;
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    // TODO: verify header is link size, require match of body size.
    ////Link count{};
    ////return header_.verify() && header_.get_body_count(count) &&
    ////    count == manager_.count();
    return true;
}

TEMPLATE
bool CLASS::snap() NOEXCEPT
{
    ////// TODO: call only after a restore, fails if size reduction.
    ////Link count{};
    ////return header_.get_body_count(count) && manager_.truncate(count);
    return true;
}

// query interface
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
    auto sink = creater(element.count());
    return sink && element.to_data(*sink);
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
reader_ptr CLASS::getter(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto ptr = body_.get(link_to_position(link));
    if (!ptr)
        return {};

    // Limits to single record or eof for slab (caller can remove limit).
    const auto source = std::make_shared<reader>(ptr);
    if constexpr (!is_slab) { source->set_limit(Size); }
    return source;
}

TEMPLATE
writer_ptr CLASS::creater(const Link& size) NOEXCEPT
{
    const auto link = body_.allocate(link_to_position(size));
    if (link == storage::eof)
        return {};

    const auto ptr = body_.get(link);
    if (!ptr)
        return {};

    // Limits to created records size or slab size (caller can remove limit).
    const auto limit = system::possible_narrow_cast<size_t>(
        link_to_position(size.value));

    const auto sink = std::make_shared<writer>(ptr);
    sink->set_limit(limit);
    return sink;
}

// private
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::link_to_position(const Link& link) NOEXCEPT
{
    const auto value = system::possible_narrow_cast<size_t>(link.value);
    BC_ASSERT(is_slab || !system::is_multiply_overflow(value, Size));

    if constexpr (is_slab) { return value; }
    if constexpr (!is_slab) { return value * Size; }
}

} // namespace database
} // namespace libbitcoin

#endif
