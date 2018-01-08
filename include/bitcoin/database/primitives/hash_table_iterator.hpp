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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_ITERATOR_HPP

namespace libbitcoin {
namespace database {

/// Forward iterator for a hash table.
/// TODO: generatlize to record_list and retain mutex for unlinks.
template <typename LinkType, typename Manager, typename Row>
class hash_table_iterator
{
public:
    hash_table_iterator(Manager& manager, LinkType index, shared_mutex& mutex);

    /// Next value in the result.
    void operator++();

    /// The element wrapped in a row object.
    Row operator*() const;

    /// Comparison operators.
    bool operator==(hash_table_iterator other) const;
    bool operator!=(hash_table_iterator other) const;

private:
    LinkType index_;
    Manager& manager_;
    shared_mutex& mutex_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/hash_table_iterator.ipp>

#endif
