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
#ifndef LIBBITCOIN_DATABASE_RECORD_ROW_HPP
#define LIBBITCOIN_DATABASE_RECORD_ROW_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

/**
 * Item for record_hash_table. A chained list with the key included.
 *
 * Stores the key, next index and user data.
 * With the starting item, we can iterate until the end using the
 * next_index() method.
 */
template <typename KeyType, typename LinkType, typename RecordManager>
class record_row
{
public:
    typedef byte_serializer::functor write_function;

    static const LinkType not_found = (LinkType)bc::max_uint64;
    static const size_t key_start = 0;
    static const size_t key_size = std::tuple_size<KeyType>::value;
    static const size_t link_size = sizeof(LinkType);
    static const size_t prefix_size = key_size + link_size;

    /// The uniform size of storing a record.
    static size_t size(size_t value_size);

    /// Construct for a new record.
    record_row(RecordManager& manager);

    /// Construct for an existing record.
    record_row(RecordManager& manager, LinkType link);

    /// Allocate and populate a new record.
    LinkType create(const KeyType& key, write_function write);

    /// Connect allocated/populated record.
    void link(LinkType next);

    /// True if the record key matches the parameter.
    bool equal(const KeyType& key) const;

    /// A smart pointer to the user data.
    memory_ptr data() const;

    /// Record index of next record in the list.
    LinkType next() const;

private:
    memory_ptr raw_data(size_t bytes) const;

    LinkType link_;
    RecordManager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_row.ipp>

#endif
