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

TEMPLATE
CLASS::element(const manager<Link, Size>& manage, Link value) NOEXCEPT
  : manager_(manage), link_(value)
{
}

TEMPLATE
void CLASS::advance() NOEXCEPT
{
    BC_ASSERT_MSG(!link_.is_eof(), "advancing from eof");
    link_ = get_next();
}

TEMPLATE
Link CLASS::self() const NOEXCEPT
{
    return link_;
}

TEMPLATE
Link CLASS::get_next() const NOEXCEPT
{
    const auto body = get();
    BC_ASSERT_MSG(body, "getting next from eof");

    return { array_cast<Link::size>(*body) };
}

TEMPLATE
Key CLASS::get_key() const NOEXCEPT
{
    const auto body = get(Link::size);
    BC_ASSERT_MSG(body, "getting key from eof");

    return array_cast<array_count<Key>>(*body);
}

TEMPLATE
bool CLASS::is_match(const Key& value) const NOEXCEPT
{
    const auto body = get(Link::size);
    BC_ASSERT_MSG(body, "comparing key at eof");

    BC_PUSH_WARNING(NO_UNSAFE_COPY_N)
    return std::equal(value.begin(), value.end(), body->begin());
    BC_POP_WARNING()
}

TEMPLATE
CLASS::operator bool() const NOEXCEPT
{
    return !link_.is_eof();
}

TEMPLATE
bool CLASS::operator==(const element& other) const NOEXCEPT
{
    return other.link_ == link_;
}

TEMPLATE
bool CLASS::operator!=(const element& other) const NOEXCEPT
{
    return other.link_ != link_;
}

// protected
// ----------------------------------------------------------------------------
// Obtaining memory object is considered const access despite the fact that
// memory is writeable. Non-const manager access implies memory map modify.

TEMPLATE
memory_ptr CLASS::get() const NOEXCEPT
{
    // Returns nullptr if link_ is eof.
    return manager_.get(link_);
}

TEMPLATE
memory_ptr CLASS::get(size_t offset) const NOEXCEPT
{
    auto body = get();
    if (body) body->increment(offset);
    return body;
}

} // namespace database
} // namespace libbitcoin

#endif
