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
#ifndef LIBBITCOIN_DATABASE_RECORD_LIST_ITERABLE_HPP
#define LIBBITCOIN_DATABASE_RECORD_LIST_ITERABLE_HPP

#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/record_list_iterator.hpp>

namespace libbitcoin {
namespace database {

/// Iterative result of a record multimap database query.
template <typename LinkType>
class record_list_iterable
{
public:
    record_list_iterable(const record_manager<LinkType>& manager,
        LinkType begin);

    record_list_iterator<LinkType> begin() const;
    record_list_iterator<LinkType> end() const;

private:
    LinkType begin_;
    const record_manager<LinkType>& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_list_iterable.ipp>

#endif
