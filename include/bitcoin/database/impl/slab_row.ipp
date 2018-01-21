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
#ifndef LIBBITCOIN_DATABASE_SLAB_ROW_IPP
#define LIBBITCOIN_DATABASE_SLAB_ROW_IPP

#include <cstddef>
#include <tuple>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>

namespace libbitcoin {
namespace database {
    
// static
template <typename KeyType, typename LinkType, typename SlabManager>
size_t slab_row<KeyType, LinkType, SlabManager>::size(size_t value_size)
{
    return std::tuple_size<KeyType>::value + sizeof(LinkType) + value_size;
}

// Parameterizing SlabManager allows const and non-const.
template <typename KeyType, typename LinkType, typename SlabManager>
slab_row<KeyType, LinkType, SlabManager>::slab_row(SlabManager& manager)
  : manager_(manager), link_(not_found)
{
}

template <typename KeyType, typename LinkType, typename SlabManager>
slab_row<KeyType, LinkType, SlabManager>::slab_row(SlabManager& manager,
    LinkType link)
  : manager_(manager), link_(link)
{
}

template <typename KeyType, typename LinkType, typename SlabManager>
LinkType slab_row<KeyType, LinkType, SlabManager>::create(const KeyType& key,
    write_function write)
{
    BITCOIN_ASSERT(link_ == not_found);

    // Create new (unlinked) record and populate its key and data.
    // [ KeyType  ] <=
    // [ LinkType ]
    // [ value... ] <=

    link_ = manager_.allocate(1);

    const auto memory = raw_data(key_start);
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_forward(key);
    serial.skip(link_size);
    serial.write_delegated(write);

    return link_;
}

template <typename KeyType, typename LinkType, typename SlabManager>
LinkType slab_row<KeyType, LinkType, SlabManager>::create(const KeyType& key,
    write_function write, size_t value_size)
{
    BITCOIN_ASSERT(link_ == not_found);

    // Create new (unlinked) slab and populate its key and data.
    // [ KeyType  ] <=
    // [ LinkType ]
    // [ value... ] <=

    const size_t slab_size = prefix_size + value_size;
    link_ = manager_.allocate(slab_size);

    const auto memory = raw_data(key_start);
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_forward(key);
    serial.skip(link_size);
    serial.write_delegated(write);

    return link_;
}

template <typename KeyType, typename LinkType, typename SlabManager>
void slab_row<KeyType, LinkType, SlabManager>::link(LinkType next)
{
    // Populate next link value.
    // [ KeyType  ]
    // [ LinkType ] <=
    // [ value... ]

    const auto memory = raw_data(key_size);
    auto serial = make_unsafe_serializer(memory->buffer());

    //*************************************************************************
    serial.template write_little_endian<LinkType>(next);
    //*************************************************************************
}

template <typename KeyType, typename LinkType, typename SlabManager>
bool slab_row<KeyType, LinkType, SlabManager>::equal(const KeyType& key) const
{
    const auto memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), memory->buffer());
}

template <typename KeyType, typename LinkType, typename SlabManager>
memory_ptr slab_row<KeyType, LinkType, SlabManager>::data() const
{
    // Get value pointer.
    // [ KeyType  ]
    // [ LinkType ]
    // [ value... ] <=

    return raw_data(prefix_size);
}

template <typename KeyType, typename LinkType, typename SlabManager>
file_offset slab_row<KeyType, LinkType, SlabManager>::offset() const
{
    // Get value file offset.
    // [ KeyType  ]
    // [ LinkType ]
    // [ value... ] <=

    return link_ + prefix_size;
}

template <typename KeyType, typename LinkType, typename SlabManager>
LinkType slab_row<KeyType, LinkType, SlabManager>::next() const
{
    const auto memory = raw_data(key_size);

    //*************************************************************************
    return from_little_endian_unsafe<LinkType>(memory->buffer());
    //*************************************************************************
}

template <typename KeyType, typename LinkType, typename SlabManager>
memory_ptr slab_row<KeyType, LinkType, SlabManager>::raw_data(
    size_t bytes) const
{
    auto memory = manager_.get(link_);
    memory->increment(bytes);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
