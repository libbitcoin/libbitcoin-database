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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_UNSAFE_COPY_N)

TEMPLATE
CLASS::element(const manager<Link, Size>& manage, const Link& start,
    const Key& key) NOEXCEPT
  : manager_(manage), link_(start), key_(key)
{
}

TEMPLATE
bool CLASS::next() NOEXCEPT
{
    while (!link_.is_terminal())
    {
        link_ = get_next();
        if (is_match())
            return true;
    }
    
    return false;
}

TEMPLATE
Link CLASS::self() NOEXCEPT
{
    if (!is_match())
        next();

    return link_;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_match() const NOEXCEPT
{
    const auto body = get(Link::size);
    if (!body) return false;
    return std::equal(key_.begin(), key_.end(), body->begin());
}

TEMPLATE
Link CLASS::get_next() const NOEXCEPT
{
    if (link_.is_terminal()) return link_;
    auto body = manager_.get(link_);
    if (!body) return Link::terminal;
    return { array_cast<Link::size>(*body) };
}

TEMPLATE
memory_ptr CLASS::get(size_t offset) const NOEXCEPT
{
    auto body = manager_.get(link_);
    if (body) body->increment(offset);
    return body;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
