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
#ifndef LIBBITCOIN_DATABASE_LINKED_LIST_ITERATOR_IPP
#define LIBBITCOIN_DATABASE_LINKED_LIST_ITERATOR_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/primitives/linked_list.hpp>

namespace libbitcoin {
namespace database {

// This class is a friend of the linked_list class.
template <typename Manager, typename Link, typename Key>
linked_list_iterator<Manager, Link, Key>::linked_list_iterator(
    Manager& manager, Link first, shared_mutex& mutex)
  : element_(manager, first, mutex)
{
}

template <typename Manager, typename Link, typename Key>
linked_list_iterator<Manager, Link, Key>&
linked_list_iterator<Manager, Link, Key>::operator++()
{
    element_.link_ = element_.next();
    return *this;
}

template <typename Manager, typename Link, typename Key>
linked_list_iterator<Manager, Link, Key>
linked_list_iterator<Manager, Link, Key>::operator++(int)
{
    auto copy = *this;
    element_.link_ = element_.next();
    return copy;
}

template <typename Manager, typename Link, typename Key>
typename const linked_list_iterator<Manager, Link, Key>::value_type&
linked_list_iterator<Manager, Link, Key>::operator*() const
{
    return element_;
}

template <typename Manager, typename Link, typename Key>
typename const linked_list_iterator<Manager, Link, Key>::value_type&
linked_list_iterator<Manager, Link, Key>::operator->() const
{
    return element_;
}

template <typename Manager, typename Link, typename Key>
bool linked_list_iterator<Manager, Link, Key>::operator==(
    const linked_list_iterator& other) const
{
    return element_ == other.element_;
}

template <typename Manager, typename Link, typename Key>
bool linked_list_iterator<Manager, Link, Key>::operator!=(
    const linked_list_iterator& other) const
{
    return element_ != other.element_;
}

} // namespace database
} // namespace libbitcoin

#endif
