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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP

#include <cstring>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename IndexType, typename LinkType>
template <typename KeyType>
inline IndexType hash_table_header<IndexType, LinkType>::remainder(
    const KeyType& key, IndexType divisor)
{
    // TODO: implement std::hash replacement to prevent store drift.
    return divisor == 0 ? 0 : std::hash<KeyType>()(key) % divisor;
}

template <typename IndexType, typename LinkType>
hash_table_header<IndexType, LinkType>::hash_table_header(storage& file,
    IndexType buckets)
  : file_(file), buckets_(buckets)
{
    static_assert(std::is_unsigned<LinkType>::value,
        "Hash table header requires unsigned value type.");

    static_assert(std::is_unsigned<IndexType>::value,
        "Hash table header requires unsigned index type.");
}

template <typename IndexType, typename LinkType>
bool hash_table_header<IndexType, LinkType>::create()
{
    const auto file_size = size(buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.resize(file_size);

    // Speed-optimized fill implementation.
    memset(memory->buffer(), (uint8_t)empty, file_size);

    // Overwrite the start of the buffer with the bucket count.
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_little_endian<IndexType>(buckets_);
    return true;
}

template <typename IndexType, typename LinkType>
bool hash_table_header<IndexType, LinkType>::start()
{
    // File is too small for the number of buckets in the header.
    if (file_.size() < offset(buckets_))
        return false;

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();

    // Does not require atomicity (no concurrency during start).
    auto deserial = make_unsafe_deserializer(memory->buffer());
    return deserial.read_little_endian<IndexType>() == buckets_;
}

template <typename IndexType, typename LinkType>
LinkType hash_table_header<IndexType, LinkType>::read(IndexType index) const
{
    BITCOIN_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto address = memory->buffer() + offset(index);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);
    return from_little_endian_unsafe<LinkType>(address);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename IndexType, typename LinkType>
void hash_table_header<IndexType, LinkType>::write(IndexType index,
    LinkType value)
{
    BITCOIN_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto address = memory->buffer() + offset(index);
    auto serial = make_unsafe_serializer(address);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);
    serial.template write_little_endian<LinkType>(value);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename IndexType, typename LinkType>
IndexType hash_table_header<IndexType, LinkType>::buckets() const
{
    return buckets_;
}

template <typename IndexType, typename LinkType>
size_t hash_table_header<IndexType, LinkType>::size()
{
    return size(buckets_);
}

// static
template <typename IndexType, typename LinkType>
size_t hash_table_header<IndexType, LinkType>::size(IndexType buckets)
{
    // Header byte size is file offset of last bucket + 1:
    //
    //  [  size:buckets        ]
    //  [ [ row[0]           ] ]
    //  [ [      ...         ] ]
    //  [ [ row[buckets - 1] ] ] <=
    //  
    return offset(buckets);
}

// static
template <typename IndexType, typename LinkType>
file_offset hash_table_header<IndexType, LinkType>::offset(IndexType index)
{
    // File offset of indexed bucket is:
    //
    //     [  size       :IndexType  ]
    //     [ [ row[0]    :LinkType ] ]
    //     [ [      ...            ] ]
    //  => [ [ row[index]:LinkType ] ]
    //
    return sizeof(IndexType) + index * sizeof(LinkType);
}

} // namespace database
} // namespace libbitcoin

#endif
