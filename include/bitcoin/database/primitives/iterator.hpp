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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/element.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

/// Element forward iterator.
template <typename Manager, typename Key, typename Link,
    if_key<Key> = true, if_link<Link> = true>
class iterator
{
public:
    // std::iterator_traits
    using iterator_category = std::output_iterator_tag;
    using value_type = element<Manager, Key, Link>;
    using difference_type = ptrdiff_t;
    using pointer = const value_type&;
    using reference = const value_type&;

    iterator(const value_type& element) NOEXCEPT;
    iterator(Manager& manager, Link start) NOEXCEPT;

    iterator& operator++() NOEXCEPT;
    iterator operator++(int) NOEXCEPT;
    pointer operator*() const NOEXCEPT;
    reference operator->() const NOEXCEPT;
    bool operator==(const iterator& other) const NOEXCEPT;
    bool operator!=(const iterator& other) const NOEXCEPT;

private:
    static constexpr auto link_size = sizeof(Link);
    static constexpr auto key_size = array_count<Key>;

    value_type element_;
};

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Manager, typename Key, typename Link,\
if_key<Key> If1, if_link<Link> If2>
#define CLASS iterator<Manager, Key, Link, If1, If2>

#include <bitcoin/database/impl/primitives/iterator.ipp>

#undef CLASS
#undef TEMPLATE

#endif
