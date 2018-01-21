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
#ifndef LIBBITCOIN_DATABASE_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_ITERATOR_HPP

namespace libbitcoin {
namespace database {

template <typename Manager, typename Link>
class iterator
{
public:
    iterator(const Manager& manager, Link index);

    void operator++();
    Link operator*() const;
    bool operator==(iterator other) const;
    bool operator!=(iterator other) const;

private:
    Link index_;
    const Manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/iterator.ipp>

#endif
