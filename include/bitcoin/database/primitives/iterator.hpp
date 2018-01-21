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

#include <cstdint>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {
    
// TODO: replace with hash_table_iterator.
/// Forward iterator for record multimap database query result.
template <typename LinkType>
class iterator
{
public:
    iterator(const record_manager<LinkType>& manager,
        LinkType index);

    /// Next value in the result.
    void operator++();

    /// The record index.
    LinkType operator*() const;

    /// Comparison operators.
    bool operator==(iterator other) const;
    bool operator!=(iterator other) const;

private:
    LinkType index_;
    const record_manager<LinkType>& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/iterator.ipp>

#endif
