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
#ifndef LIBBITCOIN_DATABASE_TABLE_ROW_HPP
#define LIBBITCOIN_DATABASE_TABLE_ROW_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

typedef std::array<uint8_t, 0> empty_key;
static_assert(std::tuple_size<empty_key>::value == 0, "non-empty empty key");

/**
 * A hash table key-conflict row, implemented as a linked list.
 * Link is limited to 64 bytes. A default Key creates an unkeyed list.
 */
template <typename Manager, typename Link, typename Key=empty_key>
class table_row
{
public:
    typedef byte_serializer::functor write_function;

    static const size_t key_start = 0;
    static const size_t key_size = std::tuple_size<Key>::value;
    static const size_t link_size = sizeof(Link);
    static const size_t prefix_size = key_size + link_size;
    static const auto not_found = (Link)bc::max_uint64;

    /// The stored size of a value with the given size.
    static size_t size(size_t value_size);

    /// Construct for a new element.
    table_row(Manager& manager);

    /// Construct for an existing element.
    table_row(Manager& manager, Link link);

    /// Allocate and populate a new array element.
    Link create(write_function write);

    /// Allocate and populate a new record element.
    Link create(const Key& key, write_function write);

    /// Allocate and populate a new slab element.
    Link create(const Key& key, write_function write, size_t value_size);

    /// Connect allocated/populated element.
    void link(Link next);

    /// True if the element key matches the parameter.
    bool equal(const Key& key) const;

    /// A smart pointer to the user data.
    memory_ptr data() const;

    /// File offset of the user data.
    file_offset offset() const;

    /// File offset of next element in the list.
    Link next() const;

private:
    memory_ptr raw_data(size_t bytes) const;
    void populate(const Key& key, write_function write);

    Link link_;
    Manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/table_row.ipp>

#endif
