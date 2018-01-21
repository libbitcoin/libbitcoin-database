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
#ifndef LIBBITCOIN_DATABASE_ITERABLE_IPP
#define LIBBITCOIN_DATABASE_ITERABLE_IPP

#include <bitcoin/database/primitives/iterator.hpp>
#include <bitcoin/database/primitives/linked_list.hpp>

namespace libbitcoin {
namespace database {

template <typename Manager, typename Link>
iterable<Manager, Link>::iterable(const Manager& manager, Link begin)
  : begin_(begin), manager_(manager)
{
}

template <typename Manager, typename Link>
bool iterable<Manager, Link>::empty() const
{
    return begin() == end();
}

template <typename Manager, typename Link>
Link iterable<Manager, Link>::front() const
{
    return *begin();
}

template <typename Manager, typename Link>
iterator<Manager, Link> iterable<Manager, Link>::begin() const
{
    return { manager_, begin_ };
}

template <typename Manager, typename Link>
iterator<Manager, Link> iterable<Manager, Link>::end() const
{
    return { manager_, linked_list<Manager, Link>::not_found };
}

} // namespace database
} // namespace libbitcoin

#endif
