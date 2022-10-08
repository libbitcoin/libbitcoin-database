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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_LIST_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_LIST_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::list(Manager& manager, Link first, shared_mutex& mutex) NOEXCEPT
  : first_(first), manager_(manager), mutex_(mutex)
{
}

TEMPLATE
bool CLASS::empty() const NOEXCEPT
{
    return begin() == end();
}

TEMPLATE
typename CLASS::const_value_type
CLASS::front() const NOEXCEPT
{
    return *begin();
}

TEMPLATE
typename CLASS::const_iterator
CLASS::begin() const NOEXCEPT
{
    return { manager_, first_, mutex_ };
}

TEMPLATE
typename CLASS::const_iterator
CLASS::end() const NOEXCEPT
{
    return { manager_, const_value_type::not_found, mutex_ };
}

} // namespace database
} // namespace libbitcoin

#endif
