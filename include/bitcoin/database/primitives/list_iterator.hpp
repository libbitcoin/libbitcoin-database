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
#ifndef LIBBITCOIN_DATABASE_LIST_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_LIST_ITERATOR_HPP

#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/list_element.hpp>

namespace libbitcoin {
namespace database {

/// Const iterator for list_element.
/// Manager dynamically traverses store-based list.
/// Mutex provides read safety for link traversal during unlink.
template <typename Manager, typename Link, typename Key>
class list_iterator
{
public:
    typedef list_element<Manager, Link, Key> value_type;
    typedef list_element<const Manager, Link, Key> const_value_type;

    /// Create a storage iterator starting at first.
    list_iterator(Manager& manager, Link first, shared_mutex& mutex);

    list_iterator& operator++();
    list_iterator operator++(int);
    const value_type& operator*() const;
    const value_type& operator->() const;
    bool operator==(const list_iterator& other) const;
    bool operator!=(const list_iterator& other) const;

private:
    value_type element_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/list_iterator.ipp>

#endif