/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_LIST_IPP
#define LIBBITCOIN_DATABASE_LIST_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/list_iterator.hpp>

namespace libbitcoin {
namespace database {

template <typename Manager, typename Link, typename Key>
list<Manager, Link, Key>::list(Manager& manager, Link first,
    shared_mutex& mutex)
  : first_(first), manager_(manager), mutex_(mutex)
{
}

template <typename Manager, typename Link, typename Key>
bool list<Manager, Link, Key>::empty() const
{
    return begin() == end();
}

template <typename Manager, typename Link, typename Key>
typename list<Manager, Link, Key>::const_value_type
list<Manager, Link, Key>::front() const
{
    return *begin();
}

template <typename Manager, typename Link, typename Key>
typename list<Manager, Link, Key>::const_iterator
list<Manager, Link, Key>::begin() const
{
    return { manager_, first_, mutex_ };
}

template <typename Manager, typename Link, typename Key>
typename list<Manager, Link, Key>::const_iterator
list<Manager, Link, Key>::end() const
{
    return { manager_, const_value_type::not_found, mutex_ };
}

} // namespace database
} // namespace libbitcoin

#endif
