/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::advance() NOEXCEPT
{
    if (link_ == eof)
        return false;

    const auto memory = get();
    link_ = system::unsafe_from_little_endian<link>(memory->data());
    return true;
}

TEMPLATE
typename CLASS::link CLASS::self() const NOEXCEPT
{
    return link_;
}

TEMPLATE
CLASS::operator bool() const NOEXCEPT
{
    return link_ != eof;
}

TEMPLATE
bool CLASS::operator==(element other) const NOEXCEPT
{
    return link_ == other.link_;
}

TEMPLATE
bool CLASS::operator!=(element other) const NOEXCEPT
{
    return link_ != other.link_;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::element(Manager& manager, link value) NOEXCEPT
  : manager_(manager), link_(value)
{
}

TEMPLATE
memory_ptr CLASS::get() const NOEXCEPT
{
    return manager_.get(link_);
}

TEMPLATE
memory_ptr CLASS::get(size_t offset) const NOEXCEPT
{
    auto memory = manager_.get(link_);
    memory->increment(offset);
    return memory;
}

TEMPLATE
memory_ptr CLASS::allocate(size_t size) const NOEXCEPT
{
    link_ = manager_.allocate(size);
    return manager_.get(link_);
}

} // namespace database
} // namespace libbitcoin

#endif
