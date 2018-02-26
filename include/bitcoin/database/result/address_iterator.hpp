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
#ifndef LIBBITCOIN_DATABASE_ADDRESS_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_ADDRESS_ITERATOR_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

class BCD_API address_iterator
{
public:
    // Definition for underlying type (avoids circular reference).
    //-------------------------------------------------------------------------
    typedef list_element<const record_manager<array_index>, array_index,
        empty_key> const_element;

    // std::iterator_traits
    //-------------------------------------------------------------------------

    typedef chain::payment_record pointer;
    typedef chain::payment_record reference;
    typedef chain::payment_record value_type;
    typedef ptrdiff_t difference_type;
    typedef std::output_iterator_tag iterator_category;
    typedef address_iterator iterator;
    typedef address_iterator const_iterator;

    // Constructors.
    //-------------------------------------------------------------------------

    address_iterator(const const_element& element);

    // Operators.
    //-------------------------------------------------------------------------

    pointer operator->() const;
    reference operator*() const;
    address_iterator& operator++();
    address_iterator operator++(int);
    bool operator==(const address_iterator& other) const;
    bool operator!=(const address_iterator& other) const;

private:
    void populate();

    const_element element_;
    value_type payment_;
};

} // namespace database
} // namespace libbitcoin

#endif
