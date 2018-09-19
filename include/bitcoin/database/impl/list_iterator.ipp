/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_LIST_ITERATOR_IPP
#define LIBBITCOIN_DATABASE_LIST_ITERATOR_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/primitives/list_element.hpp>

namespace libbitcoin {
namespace database {

template <typename Manager, typename Link, typename Key>
list_iterator<Manager, Link, Key>::list_iterator(value_type element)
  : element_(element)
{
}

template <typename Manager, typename Link, typename Key>
list_iterator<Manager, Link, Key>::list_iterator(Manager& manager,
    Link first, shared_mutex& mutex)
  : element_(manager, first, mutex)
{
}

template <typename Manager, typename Link, typename Key>
list_iterator<Manager, Link, Key>&
list_iterator<Manager, Link, Key>::operator++()
{
    element_.jump_next();
    return *this;
}

template <typename Manager, typename Link, typename Key>
list_iterator<Manager, Link, Key>
list_iterator<Manager, Link, Key>::operator++(int)
{
    auto copy = *this;
    element_.jump_next();
    return copy;
}

template <typename Manager, typename Link, typename Key>
typename list_iterator<Manager, Link, Key>::pointer
list_iterator<Manager, Link, Key>::operator*() const
{
    return element_;
}

template <typename Manager, typename Link, typename Key>
typename list_iterator<Manager, Link, Key>::reference
list_iterator<Manager, Link, Key>::operator->() const
{
    return element_;
}

template <typename Manager, typename Link, typename Key>
bool list_iterator<Manager, Link, Key>::operator==(
    const list_iterator& other) const
{
    return element_ == other.element_;
}

template <typename Manager, typename Link, typename Key>
bool list_iterator<Manager, Link, Key>::operator!=(
    const list_iterator& other) const
{
    return element_ != other.element_;
}

} // namespace database
} // namespace libbitcoin

#endif
