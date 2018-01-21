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
#ifndef LIBBITCOIN_DATABASE_ITERATOR_IPP
#define LIBBITCOIN_DATABASE_ITERATOR_IPP

#include <bitcoin/database/primitives/linked_list.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

template <typename LinkType>
iterator<LinkType>::iterator(
    const record_manager<LinkType>& manager, LinkType index)
  : index_(index), manager_(manager)
{
}

template <typename LinkType>
void iterator<LinkType>::operator++()
{
    index_ = linked_list<const record_manager<LinkType>, LinkType>(manager_,
        index_).next();
}

template <typename LinkType>
LinkType iterator<LinkType>::operator*() const
{
    return index_;
}

template <typename LinkType>
bool iterator<LinkType>::operator==(
    iterator other) const
{
    return this->index_ == other.index_;
}

template <typename LinkType>
bool iterator<LinkType>::operator!=(
    iterator other) const
{
    return this->index_ != other.index_;
}

} // namespace database
} // namespace libbitcoin

#endif
