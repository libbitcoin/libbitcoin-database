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

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    using namespace system;
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream<memory> stream{ ptr->data(), ptr->size() };
    reader source{ stream };
    if constexpr (!is_slab) { source.set_limit(Size); }
    return element.from_data(source);
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
    using namespace system;
    const auto count = element.count();
    link = manager_.allocate(count);
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream<memory> stream{ ptr->data(), ptr->size() };
    flipper sink{ stream };
    if constexpr (!is_slab) { sink.set_limit(Size * count); }
    return element.to_data(sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::put_link(const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, element) ? link : Link{};
}

} // namespace database
} // namespace libbitcoin

#endif
