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
#ifndef LIBBITCOIN_DATABASE_RECORD_MULTIMAP_ITERABLE_IPP
#define LIBBITCOIN_DATABASE_RECORD_MULTIMAP_ITERABLE_IPP

#include <bitcoin/database/primitives/record_list.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/record_list_iterator.hpp>

namespace libbitcoin {
namespace database {

template <typename LinkType>
record_list_iterable<LinkType>::record_list_iterable(
    const record_manager<LinkType>& manager, LinkType begin)
  : begin_(begin), manager_(manager)
{
}

template <typename LinkType>
record_list_iterator<LinkType> record_list_iterable<LinkType>::begin() const
{
    return record_list_iterator<LinkType>(manager_, begin_);
}

template <typename LinkType>
record_list_iterator<LinkType> record_list_iterable<LinkType>::end() const
{
    return record_list_iterator<LinkType>(manager_,
        record_list<LinkType>::empty);
}

} // namespace database
} // namespace libbitcoin

#endif
