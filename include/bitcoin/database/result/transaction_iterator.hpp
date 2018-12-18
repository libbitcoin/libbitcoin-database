/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_ITERATOR_HPP

#include <cstddef>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

class BCD_API transaction_iterator
{
public:
    // Definition for constructor type.
    //-------------------------------------------------------------------------
    typedef record_manager<array_index> manager;

    // std::iterator_traits
    //-------------------------------------------------------------------------

    typedef file_offset pointer;
    typedef file_offset reference;
    typedef file_offset value_type;
    typedef ptrdiff_t difference_type;
    typedef std::output_iterator_tag iterator_category;
    typedef transaction_iterator iterator;
    typedef transaction_iterator const_iterator;

    // Constructors.
    //-------------------------------------------------------------------------

    transaction_iterator(const manager& records, array_index start,
        size_t count);

    // Operators.
    //-------------------------------------------------------------------------

    pointer operator->() const;
    reference operator*() const;
    transaction_iterator& operator++();
    transaction_iterator operator++(int);
    bool operator==(const transaction_iterator& other) const;
    bool operator!=(const transaction_iterator& other) const;

private:
    size_t index_;
    std::vector<value_type> offsets_;
};

} // namespace database
} // namespace libbitcoin

#endif
