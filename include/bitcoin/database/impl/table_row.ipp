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
template <typename Manager, typename Link, typename Key>
size_t table_row<Manager, Link, Key>::size(size_t value_size)
{
    return std::tuple_size<Key>::value + sizeof(Link) + value_size;
}

// Parameterizing Manager allows const and non-const.
template <typename Manager, typename Link, typename Key>
table_row<Manager, Link, Key>::table_row(Manager& manager)
  : manager_(manager), link_(not_found)
{
}

template <typename Manager, typename Link, typename Key>
table_row<Manager, Link, Key>::table_row(Manager& manager, Link link)
  : manager_(manager), link_(link)
{
}

template <typename Manager, typename Link, typename Key>
void table_row<Manager, Link, Key>::populate(const Key& key,
    write_function write)
{
    // Populate a new (unlinked) element with key and value data.
    // [ Key  ] <=
    // [ Link ]
    // [ value... ] <=

    const auto memory = raw_data(key_start);
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_forward(key);
    serial.skip(link_size);
    serial.write_delegated(write);
}

// This call assumes the manager is a record_manager.
template <typename Manager, typename Link, typename Key>
Link table_row<Manager, Link, Key>::create(write_function write)
{
    static BC_CONSTEXPR empty_key unkeyed{};
    BITCOIN_ASSERT(link_ == not_found);
    link_ = manager_.allocate(1);
    populate(unkeyed, write);
    return link_;
}

// This call assumes the manager is a record_manager.
template <typename Manager, typename Link, typename Key>
Link table_row<Manager, Link, Key>::create(const Key& key,
    write_function write)
{
    BITCOIN_ASSERT(link_ == not_found);
    link_ = manager_.allocate(1);
    populate(key, write);
    return link_;
}

// This call assumes the manager is a slab_manager.
template <typename Manager, typename Link, typename Key>
Link table_row<Manager, Link, Key>::create(const Key& key,
    write_function write, size_t value_size)
{
    BITCOIN_ASSERT(link_ == not_found);
    link_ = manager_.allocate(prefix_size + value_size);
    populate(key, write);
    return link_;
}

template <typename Manager, typename Link, typename Key>
void table_row<Manager, Link, Key>::link(Link next)
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

template <typename Manager, typename Link, typename Key>
bool table_row<Manager, Link, Key>::equal(const Key& key) const
{
    const auto memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), memory->buffer());
}

template <typename Manager, typename Link, typename Key>
memory_ptr table_row<Manager, Link, Key>::data() const
{
    // Get value pointer.
    // [ Key  ]
    // [ Link ]
    // [ value... ] <=

    return raw_data(prefix_size);
}

template <typename Manager, typename Link, typename Key>
file_offset table_row<Manager, Link, Key>::offset() const
{
    // Get value file offset.
    // [ Key  ]
    // [ Link ]
    // [ value... ] <=

    return link_ + prefix_size;
}

template <typename Manager, typename Link, typename Key>
Link table_row<Manager, Link, Key>::next() const
{
    const auto memory = raw_data(key_size);

    //*************************************************************************
    return from_little_endian_unsafe<Link>(memory->buffer());
    //*************************************************************************
}

template <typename Manager, typename Link, typename Key>
memory_ptr table_row<Manager, Link, Key>::raw_data(size_t bytes) const
{
    auto memory = manager_.get(link_);
    memory->increment(bytes);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
