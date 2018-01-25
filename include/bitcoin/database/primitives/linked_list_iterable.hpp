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
#ifndef LIBBITCOIN_DATABASE_LINKED_LIST_ITERABLE_HPP
#define LIBBITCOIN_DATABASE_LINKED_LIST_ITERABLE_HPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/linked_list.hpp>
#include <bitcoin/database/primitives/linked_list_iterator.hpp>

namespace libbitcoin {
namespace database {

/// Iterable wrapper for linked_list.
/// Manager dynamically traverses store-based list.
/// Mutex provides read safety for link traversal during unlink.
template <typename Manager, typename Link, typename Key>
class linked_list_iterable
{
public:
    typedef linked_list_iterator<Manager, Link, Key> iterator;
    typedef linked_list_iterator<const Manager, Link, Key> const_iterator;
    typedef typename linked_list_iterator<Manager, Link, Key>::const_value_type
        const_value_type;

    /// Create a storage iterator starting at first.
    linked_list_iterable(Manager& manager, Link first, shared_mutex& mutex);

    bool empty() const;
    const_value_type front() const;
    const_iterator begin() const;
    const_iterator end() const;

private:
    const Link first_;
    Manager& manager_;
    shared_mutex& mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/linked_list_iterable.ipp>

#endif
