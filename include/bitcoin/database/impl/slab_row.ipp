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
#ifndef LIBBITCOIN_DATABASE_SLAB_LIST_IPP
#define LIBBITCOIN_DATABASE_SLAB_LIST_IPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/**
 * Item for slab_hash_table. A chained list with the key included.
 *
 * Stores the key, next position and user data.
 * With the starting item, we can iterate until the end using the
 * next_position() method.
 */
template <typename KeyType>
class slab_row
{
public:
    static BC_CONSTEXPR size_t position_size = sizeof(file_offset);
    static BC_CONSTEXPR size_t key_start = 0;
    static BC_CONSTEXPR size_t key_size = std::tuple_size<KeyType>::value;
    static BC_CONSTEXPR file_offset prefix_size = key_size + position_size;

    typedef serializer<uint8_t*>::functor write_function;

    slab_row(slab_manager& manager, file_offset position=0);

    /// Allocate unlinked item for the given key.
    file_offset create(const KeyType& key, write_function write,
        size_t value_size);

    /// Link allocated/populated item.
    void link(file_offset next);

    /// Does this match?
    bool compare(const KeyType& key) const;

    /// The actual user data.
    memory_ptr data() const;

    /// The file offset of the user data.
    file_offset offset() const;

    /// Position of next item in the chained list.
    file_offset next_position() const;

    /// Write a new next position.
    void write_next_position(file_offset next);

private:
    memory_ptr raw_data(file_offset offset) const;

    file_offset position_;
    slab_manager& manager_;
    mutable shared_mutex mutex_;
};

template <typename KeyType>
slab_row<KeyType>::slab_row(slab_manager& manager, file_offset position)
  : manager_(manager), position_(position)
{
    static_assert(position_size == 8, "Invalid file_offset size.");
}

template <typename KeyType>
file_offset slab_row<KeyType>::create(const KeyType& key, write_function write,
    size_t value_size)
{
    BITCOIN_ASSERT(position_ == 0);

    // Create new slab and populate its key.
    //   [ KeyType  ] <==
    //   [ next:8   ]
    //   [ value... ]
    const size_t slab_size = prefix_size + value_size;
    position_ = manager_.new_slab(slab_size);

    const auto memory = raw_data(key_start);
    const auto key_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(key_data);
    serial.write_forward(key);
    serial.skip(position_size);
    serial.write_delegated(write);

    return position_;
}

template <typename KeyType>
void slab_row<KeyType>::link(file_offset next)
{
    // Populate next pointer value.
    //   [ KeyType  ]
    //   [ next:8   ] <==
    //   [ value... ]

    // Write next pointer after the key.
    const auto memory = raw_data(key_size);
    const auto next_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(next_data);

    //*************************************************************************
    serial.template write_little_endian<file_offset>(next);
    //*************************************************************************
}

template <typename KeyType>
bool slab_row<KeyType>::compare(const KeyType& key) const
{
    const auto memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), REMAP_ADDRESS(memory));
}

template <typename KeyType>
memory_ptr slab_row<KeyType>::data() const
{
    // Get value pointer.
    //   [ KeyType  ]
    //   [ next:8   ]
    //   [ value... ] <==

    // Value data is at the end.
    return raw_data(prefix_size);
}

template <typename KeyType>
file_offset slab_row<KeyType>::offset() const
{
    // Value data is at the end.
    return position_ + prefix_size;
}

template <typename KeyType>
file_offset slab_row<KeyType>::next_position() const
{
    const auto memory = raw_data(key_size);
    const auto next_address = REMAP_ADDRESS(memory);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);
    return from_little_endian_unsafe<file_offset>(next_address);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename KeyType>
void slab_row<KeyType>::write_next_position(file_offset next)
{
    const auto memory = raw_data(key_size);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(memory));

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);
    serial.template write_little_endian<file_offset>(next);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename KeyType>
memory_ptr slab_row<KeyType>::raw_data(file_offset offset) const
{
    auto memory = manager_.get(position_);
    REMAP_INCREMENT(memory, offset);
    return memory;
}

} // namespace database
} // namespace libbitcoin

#endif
