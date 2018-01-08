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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_ITERATOR_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_ITERATOR_IPP

namespace libbitcoin {
namespace database {

template <typename LinkType, typename Manager, typename Row>
hash_table_iterator<LinkType, Manager, Row>::hash_table_iterator(
    Manager& manager, LinkType index, shared_mutex& mutex)
  : index_(index), manager_(manager), mutex_(mutex)
{
}

template <typename LinkType, typename Manager, typename Row>
void hash_table_iterator<LinkType, Manager, Row>::operator++()
{
    Row row(manager_, index_);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(update_mutex_);
    index_ = row.next_index();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename LinkType, typename Manager, typename Row>
Row hash_table_iterator<LinkType, Manager, Row>::operator*() const
{
    // TODO: optimize by storing as member.
    return Row(manager_, index_);
}

template <typename LinkType, typename Manager, typename Row>
bool hash_table_iterator<LinkType, Manager, Row>::operator==(
    hash_table_iterator other) const
{
    return this->index_ == other.index_;
}

template <typename LinkType, typename Manager, typename Row>
bool hash_table_iterator<LinkType, Manager, Row>::operator!=(
    hash_table_iterator other) const
{
    return this->index_ != other.index_;
}

} // namespace database
} // namespace libbitcoin

#endif
