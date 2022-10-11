/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ITERABLE_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ITERABLE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/element.hpp>
#include <bitcoin/database/primitives/iterator.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

/// Iterable wrapper for element.
template <typename Manager, typename Key, typename Link,
    if_key<Key> = true, if_link<Link> = true>
class iterable
{
public:
    typedef iterator<const Manager, Key, Link> iterator;
    typedef element<const Manager, Key, Link> value_type;

    iterable(Manager& manager, Link start) NOEXCEPT;

    bool empty() const NOEXCEPT;
    value_type front() const NOEXCEPT;
    iterator begin() const NOEXCEPT;
    iterator end() const NOEXCEPT;

private:
    const Link start_;
    const Manager& manager_;
};

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Manager, typename Key, typename Link,\
if_key<Key> If1, if_link<Link> If2>
#define CLASS iterable<Manager, Key, Link, If1, If2>

#include <bitcoin/database/impl/primitives/iterable.ipp>

#undef CLASS
#undef TEMPLATE

#endif
