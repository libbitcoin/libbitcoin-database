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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_LIST_ITERATOR_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_LIST_ITERATOR_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::list_iterator(const value_type& element) NOEXCEPT
  : element_(element)
{
}

TEMPLATE
CLASS::list_iterator(Manager& manager, Link first,
    shared_mutex& mutex) NOEXCEPT
  : element_(manager, first, mutex)
{
}

TEMPLATE
CLASS& CLASS::operator++() NOEXCEPT
{
    element_.jump_next();
    return *this;
}

TEMPLATE
CLASS CLASS::operator++(int) NOEXCEPT
{
    auto copy = *this;
    element_.jump_next();
    return copy;
}

TEMPLATE
typename CLASS::pointer CLASS::operator*() const NOEXCEPT
{
    return element_;
}

TEMPLATE
typename CLASS::reference CLASS::operator->() const NOEXCEPT
{
    return element_;
}

TEMPLATE
bool CLASS::operator==(const list_iterator& other) const NOEXCEPT
{
    return element_ == other.element_;
}

TEMPLATE
bool CLASS::operator!=(const list_iterator& other) const NOEXCEPT
{
    return element_ != other.element_;
}

} // namespace database
} // namespace libbitcoin

#endif
