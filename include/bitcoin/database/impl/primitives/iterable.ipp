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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ITERABLE_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ITERABLE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::iterable(Manager& manager, link start) NOEXCEPT
  : manager_(manager), start_(start)
{
}

TEMPLATE
typename CLASS::value_type
CLASS::front() const NOEXCEPT
{
    return *begin();
}

TEMPLATE
typename CLASS::iterator
CLASS::begin() const NOEXCEPT
{
    return { manager_, start_ };
}

TEMPLATE
typename CLASS::iterator
const CLASS::end() const NOEXCEPT
{
    // manager_ is not const, so recomputed at each iteration if not static.
    static const iterator stop{ manager_, value_type::eof };
    return stop;
}

TEMPLATE
bool CLASS::empty() const NOEXCEPT
{
    return begin() == end();
}

} // namespace database
} // namespace libbitcoin

#endif
