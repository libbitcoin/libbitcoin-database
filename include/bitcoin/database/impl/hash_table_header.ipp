/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP

#include <cstring>
#include <stdexcept>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

template <typename IndexType, typename ValueType>
hash_table_header<IndexType, ValueType>::hash_table_header(memory_map& file,
    IndexType buckets)
  : file_(file), buckets_(buckets)
{
    static_assert(std::is_unsigned<ValueType>::value,
        "Hash table header requires unsigned type.");
}

template <typename IndexType, typename ValueType>
void hash_table_header<IndexType, ValueType>::create()
{
    // Calculate the minimum file size.
    const auto minimum_file_size = item_position(buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.allocate(minimum_file_size);
    const auto buckets_address = ADDRESS(memory);
    auto serial = make_serializer(buckets_address);
    serial.write_little_endian(buckets_);

    // optimized fill implementation
    BITCOIN_ASSERT(empty == static_cast<ValueType>(0xffffffff));
    const auto start = buckets_address + sizeof(IndexType);
    memset(start, 0xff, buckets_ * sizeof(ValueType));

    // rationalized fill implementation
    ////for (IndexType index = 0; index < buckets_; ++index)
    ////    serial.write_little_endian(empty);
}

template <typename IndexType, typename ValueType>
void hash_table_header<IndexType, ValueType>::start()
{
    const auto minimum_file_size = item_position(buckets_);

    if (minimum_file_size > file_.size())
        throw std::runtime_error("Header file is too small.");

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto buckets_address = ADDRESS(memory);
    const auto buckets = from_little_endian_unsafe<IndexType>(buckets_address);

    if (buckets != buckets_)
        throw std::runtime_error("Header file indicates incorrect size.");
}

template <typename IndexType, typename ValueType>
ValueType hash_table_header<IndexType, ValueType>::read(IndexType index) const
{
    // This is not runtime safe but test is avoided as an optimization.
    BITCOIN_ASSERT(index < buckets_);
    
    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto value_address = ADDRESS(memory) + item_position(index);
    return from_little_endian_unsafe<ValueType>(value_address);
}

template <typename IndexType, typename ValueType>
void hash_table_header<IndexType, ValueType>::write(IndexType index,
    ValueType value)
{
    // This is not runtime safe but test is avoided as an optimization.
    BITCOIN_ASSERT(index < buckets_);
    
    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto value_position = ADDRESS(memory) + item_position(index);

    // MUST BE ATOMIC
    auto serial = make_serializer(value_position);
    serial.write_little_endian(value);
}

template <typename IndexType, typename ValueType>
IndexType hash_table_header<IndexType, ValueType>::size() const
{
    return buckets_;
}

template <typename IndexType, typename ValueType>
file_offset hash_table_header<IndexType, ValueType>::item_position(
    IndexType index) const
{
    return sizeof(IndexType) + index * sizeof(ValueType);
}

} // namespace database
} // namespace libbitcoin

#endif
