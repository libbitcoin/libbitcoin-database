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
#ifndef LIBBITCOIN_DATABASE_INPUT_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_INPUT_ITERATOR_HPP

#include <cstddef>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>

namespace libbitcoin {
namespace database {

class BCD_API inpoint_iterator
{
public:
    // Definition for constructor type (avoids circular reference).
    //-------------------------------------------------------------------------
    typedef slab_manager<file_offset> manager;
    typedef list_element<const manager, file_offset, hash_digest> const_element;

    // std::iterator_traits
    //-------------------------------------------------------------------------

    typedef chain::point pointer;
    typedef chain::point reference;
    typedef chain::point value_type;
    typedef ptrdiff_t difference_type;
    typedef std::output_iterator_tag iterator_category;
    typedef inpoint_iterator iterator;
    typedef inpoint_iterator const_iterator;

    // Constructors.
    //-------------------------------------------------------------------------

    inpoint_iterator(const const_element& element);

    // Operators.
    //-------------------------------------------------------------------------

    pointer operator->() const;
    reference operator*() const;
    inpoint_iterator& operator++();
    inpoint_iterator operator++(int);
    bool operator==(const inpoint_iterator& other) const;
    bool operator!=(const inpoint_iterator& other) const;

private:
    size_t index_;
    std::vector<value_type> inpoints_;
};

} // namespace database
} // namespace libbitcoin

#endif
