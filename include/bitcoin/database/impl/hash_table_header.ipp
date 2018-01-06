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

static const uint8_t empty_byte = bc::max_uint8;

template <typename IndexType, typename ValueType>
hash_table_header<IndexType, ValueType>::hash_table_header(storage& file,
    IndexType buckets)
  : file_(file), buckets_(buckets)
{
    BITCOIN_ASSERT_MSG(empty == (ValueType)bc::max_uint64,
        "Unexpected value for empty sentinel.");

    static_assert(std::is_unsigned<ValueType>::value,
        "Hash table header requires unsigned value type.");

    static_assert(std::is_unsigned<IndexType>::value,
        "Hash table header requires unsigned index type.");
}

template <typename IndexType, typename ValueType>
bool hash_table_header<IndexType, ValueType>::create()
{
    const auto file_size = size(buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.resize(file_size);

    // Speed-optimized fill implementation.
    memset(memory->buffer(), empty_byte, file_size);

    // Overwrite the start of the buffer with the bucket count.
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.write_little_endian<IndexType>(buckets_);
    return true;
}

template <typename IndexType, typename ValueType>
bool hash_table_header<IndexType, ValueType>::start()
{
    const auto minimum_file_size = offset(buckets_);

    // Header file is too small.
    if (file_.size() < minimum_file_size)
        return false;

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();

    // Does not require atomicity (no concurrency during start).
    auto deserial = make_unsafe_deserializer(memory->buffer());
    return deserial.read_little_endian<IndexType>() == buckets_;
}

template <typename IndexType, typename ValueType>
ValueType hash_table_header<IndexType, ValueType>::read(IndexType index) const
{
    // This is not runtime safe but guard is avoided as an optimization.
    BITCOIN_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto address = memory->buffer() + offset(index);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);
    return from_little_endian_unsafe<ValueType>(address);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename IndexType, typename ValueType>
void hash_table_header<IndexType, ValueType>::write(IndexType index,
    ValueType value)
{
    // This is not runtime safe but guard is avoided as an optimization.
    BITCOIN_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto address = memory->buffer() + offset(index);
    auto serial = make_unsafe_serializer(address);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);
    serial.template write_little_endian<ValueType>(value);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename IndexType, typename ValueType>
IndexType hash_table_header<IndexType, ValueType>::buckets() const
{
    return buckets_;
}

template <typename IndexType, typename ValueType>
size_t hash_table_header<IndexType, ValueType>::size()
{
    return size(buckets_);
}

// static
template <typename IndexType, typename ValueType>
size_t hash_table_header<IndexType, ValueType>::size(IndexType buckets)
{
    // The header size is the position of the bucket count (last + 1).
    return offset(buckets);
}

// static
template <typename IndexType, typename ValueType>
file_offset hash_table_header<IndexType, ValueType>::offset(IndexType index)
{
    // Bucket count followed by array of bucket values.
    return sizeof(IndexType) + index * sizeof(ValueType);
}

} // namespace database
} // namespace libbitcoin

#endif
