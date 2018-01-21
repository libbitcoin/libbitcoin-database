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
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

template <typename LinkType>
iterable<LinkType>::iterable(
    const record_manager<LinkType>& manager, LinkType begin)
  : begin_(begin), manager_(manager)
{
}

template <typename LinkType>
iterator<LinkType> iterable<LinkType>::begin() const
{
    return iterator<LinkType>(manager_, begin_);
}

template <typename LinkType>
iterator<LinkType> iterable<LinkType>::end() const
{
    return iterator<LinkType>(manager_,
        linked_list<record_manager<LinkType>, LinkType>::not_found);
}

} // namespace database
} // namespace libbitcoin

#endif
