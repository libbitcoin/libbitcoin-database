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
#ifndef LIBBITCOIN_DATABASE_MULTIMAP_ITERATOR_IPP
#define LIBBITCOIN_DATABASE_MULTIMAP_ITERATOR_IPP

#include <bitcoin/database/primitives/record_list.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

template <typename LinkType>
record_list_iterator<LinkType>::record_list_iterator(
    const record_manager& manager, LinkType index)
  : index_(index), manager_(manager)
{
}

template <typename LinkType>
void record_list_iterator<LinkType>::operator++()
{
    // HACK: next_index() is const, so this is safe despite being ugly.
    auto& manager = const_cast<record_manager&>(manager_);

    index_ = record_list<LinkType>(manager, index_).next_index();
}

template <typename LinkType>
LinkType record_list_iterator<LinkType>::operator*() const
{
    return index_;
}

template <typename LinkType>
bool record_list_iterator<LinkType>::operator==(
    record_list_iterator other) const
{
    return this->index_ == other.index_;
}

template <typename LinkType>
bool record_list_iterator<LinkType>::operator!=(
    record_list_iterator other) const
{
    return this->index_ != other.index_;
}

} // namespace database
} // namespace libbitcoin

#endif
