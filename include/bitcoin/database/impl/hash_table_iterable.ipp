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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_ITERABLE_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_ITERABLE_IPP

#include <bitcoin/database/primitives/hash_table_iterator.hpp>

namespace libbitcoin {
namespace database {

template <typename LinkType, typename Manager, typename Row>
hash_table_iterable<LinkType, Manager, Row>::hash_table_iterable(
    Manager& manager, LinkType begin, shared_mutex& mutex)
  : begin_(begin), manager_(manager), mutex_(mutex)
{
}

template <typename LinkType, typename Manager, typename Row>
hash_table_iterator<LinkType, Manager, Row>
    hash_table_iterable<LinkType>::begin() const
{
    return hash_table_iterator<LinkType, Manager, Row>(manager_, begin_, mutex_);
}

template <typename LinkType, typename Manager, typename Row>
hash_table_iterator<LinkType, Manager>
    hash_table_iterable<LinkType>::end() const
{
    return hash_table_iterator<LinkType, Manager, Row>(manager_,
        Row::not_found, mutex_);
}

} // namespace database
} // namespace libbitcoin

#endif
