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
#include <bitcoin/database/result/address_iterator.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

address_iterator::address_iterator(const const_element& element)
  : element_(element)
{
    populate();
}

void address_iterator::populate()
{
    if (element_.link() != element_.not_found)
    {
        element_.read([&](byte_deserializer& deserial)
        {
            payment_.from_data(deserial, false);
        });
    }
}

address_iterator::pointer address_iterator::operator->() const
{
    return payment_;
}

address_iterator::reference address_iterator::operator*() const
{
    return payment_;
}

address_iterator::iterator& address_iterator::operator++()
{
    populate();
    return *this;
}

address_iterator::iterator address_iterator::operator++(int)
{
    auto it = *this;
    populate();
    return it;
}

bool address_iterator::operator==(const address_iterator& other) const
{
    return element_ == other.element_;
}

bool address_iterator::operator!=(const address_iterator& other) const
{
    return !(*this == other);
}

} // namespace database
} // namespace libbitcoin
