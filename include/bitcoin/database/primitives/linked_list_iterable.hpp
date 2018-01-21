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

#include <bitcoin/database/primitives/linked_list_iterator.hpp>

namespace libbitcoin {
namespace database {

/// Iterable wrapper for linked_list.
/// Link is both traversal and value.
/// Manager dynamically traverses store-based list.
template <typename Manager, typename Link>
class linked_list_iterable
{
public:
    /// Create a storage interator starting at first.
    linked_list_iterable(const Manager& manager, Link first);

    bool empty() const;
    Link front() const;
    linked_list_iterator<Manager, Link> begin() const;
    linked_list_iterator<Manager, Link> end() const;

private:
    const Link first_;
    const Manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/linked_list_iterable.ipp>

#endif
