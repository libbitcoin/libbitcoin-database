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
#ifndef LIBBITCOIN_DATABASE_TABLE_ROW_IPP
#define LIBBITCOIN_DATABASE_TABLE_ROW_IPP

#include <cstddef>
#include <tuple>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

// static
template <typename Key, typename Link, typename Manager>
size_t table_row<Key, Link, Manager>::size(size_t value_size)
{
    return std::tuple_size<Key>::value + sizeof(Link) + value_size;
}

// Parameterizing Manager allows const and non-const.
template <typename Key, typename Link, typename Manager>
table_row<Key, Link, Manager>::table_row(Manager& manager)
  : manager_(manager), link_(not_found)
{
}

template <typename Key, typename Link, typename Manager>
table_row<Key, Link, Manager>::table_row(Manager& manager, Link link)
  : manager_(manager), link_(link)
{
}

template <typename Key, typename Link, typename Manager>
Link table_row<Key, Link, Manager>::create(const Key& key,
    write_function write)
{
    BITCOIN_ASSERT(link_ == not_found);

    // Create new (unlinked) record and populate its key and data.
    // [ Key  ] <=
    // [ Link ]
    // [ value... ] <=

    link_ = manager_.allocate(1);

    const auto memory = raw_data(key_start);
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_forward(key);
    serial.skip(link_size);
    serial.write_delegated(write);

    return link_;
}

template <typename Key, typename Link, typename Manager>
Link table_row<Key, Link, Manager>::create(const Key& key,
    write_function write, size_t value_size)
{
    BITCOIN_ASSERT(link_ == not_found);

    // Create new (unlinked) element and populate its key and data.
    // [ Key  ] <=
    // [ Link ]
    // [ value... ] <=

    const size_t element_size = prefix_size + value_size;
    link_ = manager_.allocate(element_size);

    const auto memory = raw_data(key_start);
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_forward(key);
    serial.skip(link_size);
    serial.write_delegated(write);

    return link_;
}

template <typename Key, typename Link, typename Manager>
void table_row<Key, Link, Manager>::link(Link next)
{
    // Populate next link value.
    // [ Key  ]
    // [ Link ] <=
    // [ value... ]

    const auto memory = raw_data(key_size);
    auto serial = make_unsafe_serializer(memory->buffer());

    //*************************************************************************
    serial.template write_little_endian<Link>(next);
    //*************************************************************************
}

template <typename Key, typename Link, typename Manager>
bool table_row<Key, Link, Manager>::equal(const Key& key) const
{
    const auto memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), memory->buffer());
}

template <typename Key, typename Link, typename Manager>
memory_ptr table_row<Key, Link, Manager>::data() const
{
    // Get value pointer.
    // [ Key  ]
    // [ Link ]
    // [ value... ] <=

    return raw_data(prefix_size);
}

template <typename Key, typename Link, typename Manager>
file_offset table_row<Key, Link, Manager>::offset() const
{
    // Get value file offset.
    // [ Key  ]
    // [ Link ]
    // [ value... ] <=

    return link_ + prefix_size;
}

template <typename Key, typename Link, typename Manager>
Link table_row<Key, Link, Manager>::next() const
{
    const auto memory = raw_data(key_size);

    //*************************************************************************
    return from_little_endian_unsafe<Link>(memory->buffer());
    //*************************************************************************
}

template <typename Key, typename Link, typename Manager>
memory_ptr table_row<Key, Link, Manager>::raw_data(size_t bytes) const
{
    auto memory = manager_.get(link_);
    memory->increment(bytes);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
